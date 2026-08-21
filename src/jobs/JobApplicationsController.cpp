#include "JobApplicationsController.hpp"
#include "AddJobService.hpp"
#include "JobSaveWorker.hpp"
#include "JobApplicationValidator.hpp"
#include "UpdateJobService.hpp"
#include "common/CancellationState.hpp"
#include "maintenance/DataRemovalWorker.hpp"
#include "maintenance/StorageMutationGate.hpp"

#include <QMetaType>

#include <memory>
#include <type_traits>
#include <utility>

namespace {

	JobApplicationDraft fillDraft(const QVariantMap& formValues)
	{
		JobApplicationDraft draft;
		draft.jobTitle_ = formValues.value(QStringLiteral("jobTitle")).toString();
		draft.jobUrl_ = formValues.value(QStringLiteral("jobUrl")).toString();
		draft.companyName_ = formValues.value(QStringLiteral("companyName")).toString();
		draft.workFormat_ = formValues.value(QStringLiteral("workFormat")).toString();
		draft.city_ = formValues.value(QStringLiteral("city")).toString();
		draft.salary_ = formValues.value(QStringLiteral("salary")).toString();
		draft.status_ = formValues.value(QStringLiteral("status")).toString();
		draft.appliedDate_ = formValues.value(QStringLiteral("appliedDate")).toString();
		draft.nextStep_ = formValues.value(QStringLiteral("nextStep")).toString();
		draft.description_ = formValues.value(QStringLiteral("description")).toString();
		draft.requirements_ = formValues.value(QStringLiteral("requirements")).toString();
		draft.notes_ = formValues.value(QStringLiteral("notes")).toString();

		const auto technologies = formValues.value(QStringLiteral("techStack"));
		draft.techStack_ = technologies.metaType().id() == QMetaType::QStringList
			? technologies.toStringList()
			: technologies.toString().split(',', Qt::SkipEmptyParts);
		return draft;
	}

}

JobApplicationsController::JobApplicationsController(
	QVector<JobApplication> applications,
	JobSaveWorker& jobSaveWorker,
	QObject* parent)
	: QObject{parent}
	, applicationsModel_{std::move(applications)}
	, selectionTracker_{filteredApplicationsModel_, JobApplicationListModel::IdRole}
	, bulkSelectionTracker_{filteredApplicationsModel_, JobApplicationListModel::IdRole}
	, jobSaveWorker_{jobSaveWorker}
{
	filteredApplicationsModel_.setSearchRoles({
		JobApplicationListModel::CompanyNameRole,
		JobApplicationListModel::JobTitleRole,
		JobApplicationListModel::CvFileNameRole,
		JobApplicationListModel::StatusLabelRole,
		JobApplicationListModel::NextStepRole,
		JobApplicationListModel::WorkFormatRole,
		JobApplicationListModel::SalaryRole,
		});
	filteredApplicationsModel_.setSort(JobApplicationListModel::AppliedDateValueRole, Qt::DescendingOrder);
	connect(
		&selectionTracker_,
		&StableIdSelectionTracker::selectionChanged,
		this,
		&JobApplicationsController::handleSelectionChanged);
	connect(
		&bulkSelectionTracker_,
		&BulkIdSelectionTracker::selectionChanged,
		this,
		[this]() {
			emit checkedApplicationsChanged();
			emit deletionAvailabilityChanged();
		});
	filteredApplicationsModel_.setSourceModel(&applicationsModel_);
	selectionTracker_.synchronize();
	publishedApplicationCount_ = applicationCount();
	connect(
		&filteredApplicationsModel_,
		&QAbstractItemModel::rowsInserted,
		this,
		[this]() { handleVisibleCountChanged(); });
	connect(
		&filteredApplicationsModel_,
		&QAbstractItemModel::rowsMoved,
		this,
		[this]() { handleVisibleCountChanged(); });
	connect(
		&filteredApplicationsModel_,
		&QAbstractItemModel::rowsRemoved,
		this,
		[this]() { handleVisibleCountChanged(); });
	connect(
		&filteredApplicationsModel_,
		&QAbstractItemModel::modelReset,
		this,
		[this]() { handleVisibleCountChanged(); });
	connect(
		&jobSaveWorker_,
		&JobSaveWorker::saveCompleted,
		this,
		&JobApplicationsController::handleAddJobSave);
	connect(
		&jobSaveWorker_,
		&JobSaveWorker::updateCompleted,
		this,
		&JobApplicationsController::handleUpdateJobSave);
}

JobApplicationsController::JobApplicationsController(
	QVector<JobApplication> applications,
	JobSaveWorker& jobSaveWorker,
	DataRemovalWorker& removalWorker,
	StorageMutationGate& mutationGate,
	QObject* parent)
	: JobApplicationsController(std::move(applications), jobSaveWorker, parent)
{
	removalWorker_ = &removalWorker;
	mutationGate_ = &mutationGate;
	connect(
		removalWorker_,
		&DataRemovalWorker::removalCompleted,
		this,
		&JobApplicationsController::handleRemovalCompleted);
	connect(
		mutationGate_,
		&StorageMutationGate::stateChanged,
		this,
		&JobApplicationsController::deletionAvailabilityChanged);
}

JobApplicationsController::~JobApplicationsController()
{
	shuttingDown_ = true;
	if (mutationGate_ != nullptr) {
		for (qsizetype index = 0; index < saveQueue_.size(); ++index) {
			mutationGate_->releaseJobSave();
		}
		if (activeJobSave_.has_value()) {
			mutationGate_->releaseJobSave();
		}
	}
	saveQueue_.clear();
	if (activeSaveCancellation_ != nullptr) {
		activeSaveCancellation_->requestCancellation();
	}
	if (activeDeletionCancellation_ != nullptr) {
		activeDeletionCancellation_->requestCancellation();
		if (mutationGate_ != nullptr) {
			mutationGate_->endRemoval();
		}
	}
}

QAbstractItemModel* JobApplicationsController::applicationsModel()
{
	return &filteredApplicationsModel_;
}

JobApplicationListModel& JobApplicationsController::jobApplicationListModel()
{
	return applicationsModel_;
}

const JobApplicationListModel& JobApplicationsController::jobApplicationListModel() const
{
	return applicationsModel_;
}

int JobApplicationsController::applicationCount() const
{
	return filteredApplicationsModel_.rowCount();
}

int JobApplicationsController::selectedApplicationIndex() const
{
	return selectionTracker_.selectedRow();
}

QString JobApplicationsController::selectedApplicationId() const
{
	return selectionTracker_.selectedId();
}

QVariantMap JobApplicationsController::selectedApplication() const
{
	const auto sourceIndex = selectionTracker_.selectedSourceIndex();
	return sourceIndex.isValid() ? applicationToMap(sourceIndex.row()) : QVariantMap();
}

QString JobApplicationsController::searchText() const
{
	return searchText_;
}

QString JobApplicationsController::statusFilter() const
{
	return statusFilter_;
}

QString JobApplicationsController::resultSummary() const
{
	const auto count = applicationCount();
	if (count == 0) {
		return QStringLiteral("Showing 0 applications");
	}

	return QStringLiteral("Showing 1 to %1 of %1 applications").arg(count);
}

void JobApplicationsController::selectApplication(int index)
{
	selectionTracker_.selectRow(index);
}

bool JobApplicationsController::saving() const
{
	return pendingSaveCount() > 0;
}

int JobApplicationsController::pendingSaveCount() const
{
	return static_cast<int>(saveQueue_.size())
		+ (activeJobSave_.has_value()
			? 1 : 0);
}

bool JobApplicationsController::updatingApplication() const
{
	return pendingUpdateOperationId_ != 0;
}

QStringList JobApplicationsController::checkedApplicationIds() const
{
	return bulkSelectionTracker_.selectedIds();
}

int JobApplicationsController::checkedApplicationCount() const
{
	return bulkSelectionTracker_.selectedCount();
}

bool JobApplicationsController::allVisibleApplicationsChecked() const
{
	return bulkSelectionTracker_.allVisibleSelected();
}

bool JobApplicationsController::someVisibleApplicationsChecked() const
{
	return bulkSelectionTracker_.someVisibleSelected();
}

bool JobApplicationsController::deletingApplications() const
{
	return activeDeletionCancellation_ != nullptr;
}

int JobApplicationsController::pendingDeletionCount() const
{
	return deletingApplications() ? 1 : 0;
}

bool JobApplicationsController::canDeleteApplications() const
{
	return removalWorker_ != nullptr
		&& checkedApplicationCount() > 0
		&& !deletingApplications()
		&& mutationGate_ != nullptr
		&& mutationGate_->canBeginRemoval();
}

void JobApplicationsController::setSearchText(const QString& text)
{
	const auto normalized = text.trimmed();
	if (searchText_ == normalized) {
		return;
	}

	searchText_ = normalized;
	bulkSelectionTracker_.clear();
	selectionTracker_.beginModelUpdate();
	visibleCountNotificationsSuppressed_ = true;
	filteredApplicationsModel_.setSearchText(searchText_);
	visibleCountNotificationsSuppressed_ = false;
	selectionTracker_.endModelUpdate();
	handleVisibleCountChanged();
	emit searchTextChanged();
}

void JobApplicationsController::setStatusFilter(const QString& status)
{
	const auto normalized = status.trimmed();
	if (statusFilter_ == normalized) {
		return;
	}

	statusFilter_ = normalized;
	bulkSelectionTracker_.clear();
	selectionTracker_.beginModelUpdate();
	visibleCountNotificationsSuppressed_ = true;
	if (statusFilter_.isEmpty() || statusFilter_ == QStringLiteral("All")) {
		filteredApplicationsModel_.clearExactFilter();
	}
	else {
		filteredApplicationsModel_.setExactFilter(
			JobApplicationListModel::StatusValueRole,
			QString::number(static_cast<int>(jobStatusFromString(statusFilter_))));
	}
	visibleCountNotificationsSuppressed_ = false;
	selectionTracker_.endModelUpdate();
	handleVisibleCountChanged();
	emit statusFilterChanged();
}

void JobApplicationsController::clearFilters()
{
	if (searchText_.isEmpty() && statusFilter_.isEmpty()) {
		return;
	}

	const bool didSearchTextChange = !searchText_.isEmpty();
	const bool didStatusFilterChange = !statusFilter_.isEmpty();
	searchText_.clear();
	statusFilter_.clear();
	bulkSelectionTracker_.clear();
	selectionTracker_.beginModelUpdate();
	visibleCountNotificationsSuppressed_ = true;
	filteredApplicationsModel_.setSearchText(QString());
	filteredApplicationsModel_.clearExactFilter();
	visibleCountNotificationsSuppressed_ = false;
	selectionTracker_.endModelUpdate();
	handleVisibleCountChanged();
	if (didSearchTextChange) {
		emit searchTextChanged();
	}
	if (didStatusFilterChange) {
		emit statusFilterChanged();
	}
}

QStringList JobApplicationsController::validateSelectedApplication() const
{
	if (selectedSourceApplication() == nullptr) {
		return {};
	}

	return JobApplicationValidator::validate(*selectedSourceApplication()).messages();
}

void JobApplicationsController::createApplication(
	const QVariantMap& formValues,
	const QUrl& selectedCvUrl)
{
	if (shuttingDown_) {
		return;
	}

	const auto preflight = AddJobService::preflight(fillDraft(formValues), selectedCvUrl);
	if (!preflight.isValid()) {
		emit saveFailed(preflight.fieldErrors_, preflight.message_);
		return;
	}
	if (mutationGate_ != nullptr && !mutationGate_->reserveJobSave()) {
		emit saveFailed(
			{},
			QStringLiteral("Wait for the active deletion to finish before adding a job."));
		return;
	}

	const auto operationId = ++nextSaveOperationId_;
	const auto previousCount = pendingSaveCount();
	saveQueue_.emplace_back(QueuedCreateApplication{
		operationId,
		preflight.draft_,
		selectedCvUrl});
	publishPendingSaveStateChange(previousCount);
	emit applicationQueued(operationId);
	startNextJobSave();
}

void JobApplicationsController::updateApplication(
	const QString& applicationId,
	const QVariantMap& formValues,
	const QUrl& replacementCvUrl)
{
	if (shuttingDown_) {
		return;
	}

	const auto normalizedId = applicationId.trimmed();
	const auto* const existing = sourceApplicationById(normalizedId);
	if (existing == nullptr) {
		emit applicationUpdateRejected(
			0,
			normalizedId,
			{},
			QStringLiteral("The job application no longer exists."));
		return;
	}
	if (updatingApplication()) {
		emit applicationUpdateRejected(
			0,
			normalizedId,
			{},
			QStringLiteral("Wait for the current job update to finish."));
		return;
	}

	const auto preflight = UpdateJobService::preflight(
		fillDraft(formValues),
		!existing->cvId_.trimmed().isEmpty(),
		replacementCvUrl);
	if (!preflight.isValid()) {
		emit applicationUpdateRejected(
			0,
			normalizedId,
			preflight.fieldErrors_,
			preflight.message_);
		return;
	}
	if (mutationGate_ != nullptr && !mutationGate_->reserveJobSave()) {
		emit applicationUpdateRejected(
			0,
			normalizedId,
			{},
			QStringLiteral("Wait for the active deletion to finish before updating a job."));
		return;
	}

	const auto operationId = ++nextSaveOperationId_;
	const auto previousCount = pendingSaveCount();
	pendingUpdateOperationId_ = operationId;
	pendingUpdateApplicationId_ = normalizedId;
	saveQueue_.emplace_back(QueuedUpdateApplication{
		operationId,
		normalizedId,
		preflight.draft_,
		replacementCvUrl});
	emit updatingApplicationChanged();
	publishPendingSaveStateChange(previousCount);
	emit applicationUpdateQueued(operationId, normalizedId);
	startNextJobSave();
}

void JobApplicationsController::startNextJobSave()
{
	if (shuttingDown_ || activeJobSave_.has_value() || saveQueue_.empty()) {
		return;
	}

	activeJobSave_ = std::move(saveQueue_.front());
	saveQueue_.pop_front();
	activeSaveCancellation_ = std::make_shared<CancellationState>();
	suppressActiveCompletionNotification_ = false;

	std::visit(
		[this](const auto& queued) {
			using Request = std::decay_t<decltype(queued)>;
			if constexpr (std::is_same_v<Request, QueuedCreateApplication>) {
				jobSaveWorker_.submit({
					queued.operationId_,
					queued.draft_,
					queued.selectedCvUrl_,
					activeSaveCancellation_});
			}
			else {
				jobSaveWorker_.submit({
					queued.operationId_,
					queued.applicationId_,
					queued.draft_,
					queued.replacementCvUrl_,
					activeSaveCancellation_});
			}
		},
		*activeJobSave_);
}

void JobApplicationsController::cancelCreateApplication()
{
	if (activeJobSave_.has_value()
		&& std::holds_alternative<QueuedCreateApplication>(*activeJobSave_)
		&& activeSaveCancellation_ != nullptr) {
		activeSaveCancellation_->requestCancellation();
	}
}

void JobApplicationsController::cancelAllCreateApplications()
{
	cancelAllJobSaves();
}

void JobApplicationsController::cancelAllJobSaves()
{
	const auto previousCount = pendingSaveCount();
	if (mutationGate_ != nullptr) {
		for (qsizetype index = 0; index < saveQueue_.size(); ++index) {
			mutationGate_->releaseJobSave();
		}
	}
	saveQueue_.clear();
	const bool activeUpdate = activeJobSave_.has_value()
		&& std::holds_alternative<QueuedUpdateApplication>(*activeJobSave_);
	if (!activeUpdate) {
		clearPendingUpdate();
	}
	if (activeSaveCancellation_ != nullptr) {
		suppressActiveCompletionNotification_ = true;
		activeSaveCancellation_->requestCancellation();
	}
	publishPendingSaveStateChange(previousCount);
}

void JobApplicationsController::handleAddJobSave(const AddJobSaveOutcome& outcome)
{
	if (!isActiveSaveOutcome(outcome.operationId_, outcome.cancellation_, false)) {
		return;
	}

	const auto& result = outcome.result_;

	if (result.success_) {
		applicationsModel_.appendApplication(result.application_);
		emit companyResolved(result.company_.id_, result.company_.name_);
		filteredApplicationsModel_.sort(filteredApplicationsModel_.sortColumn(), filteredApplicationsModel_.sortOrder());
		emit cvUsed(
			result.cvDocument_,
			result.application_.id_,
			result.cvImportDisposition_);
		emit applicationCreated(result.application_.id_);
	}

	if (!suppressActiveCompletionNotification_) {
		const auto message = result.message_.isEmpty()
			? (result.success_
				? QStringLiteral("Job application saved successfully.")
				: QStringLiteral("Job application could not be saved."))
			: result.message_;
		emit applicationSaveCompleted(
			outcome.operationId_,
			outcome.jobTitle_,
			result.success_,
			message);
	}
	releaseActiveJobSave();
}

void JobApplicationsController::handleUpdateJobSave(const UpdateJobSaveOutcome& outcome)
{
	if (!isActiveSaveOutcome(outcome.operationId_, outcome.cancellation_, true)
		|| outcome.applicationId_ != pendingUpdateApplicationId_) {
		return;
	}

	const auto& result = outcome.result_;
	bool success = result.success_;
	QString message = result.message_;
	if (success) {
		success = applicationsModel_.updateApplication(result.application_);
		if (success) {
			emit companyResolved(result.company_.id_, result.company_.name_);
			if (result.replacementCvDocument_.has_value()
				&& result.previousCvId_ != result.replacementCvDocument_->id_) {
				emit cvReplaced(
					result.previousCvId_,
					*result.replacementCvDocument_,
					result.application_.id_,
					result.cvImportDisposition_);
			}
		}
		else {
			message = QStringLiteral(
				"The update was saved, but the application could not be refreshed in the current view.");
		}
	}

	if (message.isEmpty()) {
		message = success
			? QStringLiteral("Job application changes saved successfully.")
			: QStringLiteral("Job application changes could not be saved.");
	}
	if (!suppressActiveCompletionNotification_) {
		emit applicationUpdateCompleted(
			outcome.operationId_,
			outcome.applicationId_,
			outcome.jobTitle_,
			success,
			result.fieldErrors_,
			message);
	}
	releaseActiveJobSave();
}

bool JobApplicationsController::isActiveSaveOutcome(
	quint64 operationId,
	const std::shared_ptr<CancellationState>& cancellation,
	bool expectUpdate) const
{
	if (shuttingDown_
		|| !activeJobSave_.has_value()
		|| activeSaveCancellation_ != cancellation) {
		return false;
	}

	const bool activeUpdate = std::holds_alternative<QueuedUpdateApplication>(
		*activeJobSave_);
	const auto activeOperationId = std::visit(
		[](const auto& queued) { return queued.operationId_; },
		*activeJobSave_);
	return activeUpdate == expectUpdate && activeOperationId == operationId;
}

void JobApplicationsController::releaseActiveJobSave()
{
	const auto previousCount = pendingSaveCount();
	const bool wasUpdate = activeJobSave_.has_value()
		&& std::holds_alternative<QueuedUpdateApplication>(*activeJobSave_);
	activeJobSave_.reset();
	activeSaveCancellation_.reset();
	if (mutationGate_ != nullptr) {
		mutationGate_->releaseJobSave();
	}
	if (wasUpdate) {
		clearPendingUpdate();
	}
	suppressActiveCompletionNotification_ = false;
	publishPendingSaveStateChange(previousCount);
	startNextJobSave();
}

void JobApplicationsController::clearPendingUpdate()
{
	if (!updatingApplication()) {
		return;
	}
	pendingUpdateOperationId_ = 0;
	pendingUpdateApplicationId_.clear();
	emit updatingApplicationChanged();
}

void JobApplicationsController::toggleApplicationChecked(int index)
{
	bulkSelectionTracker_.toggleRow(index);
}

void JobApplicationsController::setAllVisibleApplicationsChecked(bool checked)
{
	bulkSelectionTracker_.setAllVisibleSelected(checked);
}

void JobApplicationsController::clearCheckedApplications()
{
	bulkSelectionTracker_.clear();
}

void JobApplicationsController::deleteCheckedApplications()
{
	if (!canDeleteApplications() || !mutationGate_->beginRemoval()) {
		emit applicationDeletionCompleted(
			0,
			checkedApplicationCount(),
			false,
			QStringLiteral("Wait for pending save or CV import work before deleting jobs."));
		return;
	}

	QVector<DataRemovalItemRequest> items;
	for (const auto& id : checkedApplicationIds()) {
		QString label = id;
		for (int row = 0; row < applicationsModel_.rowCount(); ++row) {
			const auto* application = applicationsModel_.applicationAt(row);
			if (application != nullptr && application->id_ == id) {
				label = application->jobTitle_;
				break;
			}
		}
		items.append({id, label});
	}

	activeDeletionOperationId_ = ++nextDeletionOperationId_;
	activeDeletionCancellation_ = std::make_shared<CancellationState>();
	emit deletingApplicationsChanged();
	emit pendingDeletionCountChanged();
	emit deletionAvailabilityChanged();
	removalWorker_->submit({
		activeDeletionOperationId_,
		DataRemovalKind::DeleteJobs,
		std::move(items),
		activeDeletionCancellation_});
}

void JobApplicationsController::cancelApplicationDeletion()
{
	if (activeDeletionCancellation_ != nullptr) {
		activeDeletionCancellation_->requestCancellation();
	}
}

void JobApplicationsController::handleRemovalCompleted(
	const DataRemovalBatchOutcome& outcome)
{
	if (shuttingDown_
		|| outcome.kind_ != DataRemovalKind::DeleteJobs
		|| outcome.operationId_ != activeDeletionOperationId_
		|| outcome.cancellation_ != activeDeletionCancellation_) {
		return;
	}

	QStringList deletedIds;
	QStringList failureDetails;
	for (const auto& item : outcome.result_.items_) {
		if (item.status_ == DataRemovalItemStatus::Deleted) {
			deletedIds.append(item.id_);
		} else {
			failureDetails.append(QStringLiteral("%1: %2").arg(item.label_, item.message_));
		}
	}

	applicationsModel_.removeApplications(deletedIds);
	bulkSelectionTracker_.removeIds(deletedIds);
	if (!deletedIds.isEmpty()) {
		emit applicationsDeleted(deletedIds);
	}

	QString message = QStringLiteral("Deleted %1 job application(s).").arg(deletedIds.size());
	if (!failureDetails.isEmpty()) {
		message.append(QStringLiteral(" Failed: %1").arg(failureDetails.join(QStringLiteral("; "))));
	}
	if (outcome.result_.cancelled_) {
		message.append(QStringLiteral(" Remaining items were canceled."));
	}
	const auto failedCount = failureDetails.size();
	releaseApplicationDeletion();
	emit applicationDeletionCompleted(
		deletedIds.size(),
		failedCount,
		outcome.result_.cancelled_,
		message);
}

void JobApplicationsController::releaseApplicationDeletion()
{
	activeDeletionCancellation_.reset();
	activeDeletionOperationId_ = 0;
	if (mutationGate_ != nullptr) {
		mutationGate_->endRemoval();
	}
	emit deletingApplicationsChanged();
	emit pendingDeletionCountChanged();
	emit deletionAvailabilityChanged();
}

void JobApplicationsController::publishPendingSaveStateChange(int previousCount)
{
	const auto currentCount = pendingSaveCount();

	if (previousCount == currentCount)
		return;

	emit pendingSaveCountChanged();

	if ((previousCount == 0) != (currentCount == 0)) 
		emit savingChanged();
	
	if (previousCount > 0 && currentCount == 0 && !shuttingDown_)
		emit saveQueueDrained();
	
}

const JobApplication* JobApplicationsController::selectedSourceApplication() const
{
	const auto sourceIndex = selectionTracker_.selectedSourceIndex();
	return sourceIndex.isValid() ? applicationsModel_.applicationAt(sourceIndex.row()) : nullptr;
}

const JobApplication* JobApplicationsController::sourceApplicationById(
	const QString& applicationId) const
{
	if (applicationId.isEmpty()) {
		return nullptr;
	}
	for (int row = 0; row < applicationsModel_.rowCount(); ++row) {
		const auto* const application = applicationsModel_.applicationAt(row);
		if (application != nullptr && application->id_ == applicationId) {
			return application;
		}
	}
	return nullptr;
}

void JobApplicationsController::handleSelectionChanged(
	bool idChanged,
	bool rowChanged,
	bool dataChanged)
{
	if (idChanged) {
		emit selectedApplicationIdChanged();
	}
	if (rowChanged) {
		emit selectedApplicationIndexChanged();
	}
	if (idChanged || dataChanged) {
		emit selectedApplicationChanged();
	}
}

void JobApplicationsController::handleVisibleCountChanged()
{
	if (visibleCountNotificationsSuppressed_) {
		return;
	}

	const auto count = applicationCount();
	if (publishedApplicationCount_ == count) {
		return;
	}

	publishedApplicationCount_ = count;
	emit applicationCountChanged();
	emit resultSummaryChanged();
}

QVariantMap JobApplicationsController::applicationToMap(int sourceRow) const
{
	const auto modelIndex = applicationsModel_.index(sourceRow, 0);
	const auto roleData = [this, &modelIndex](int role) {
		return applicationsModel_.data(modelIndex, role);
		};
	return {
		{QStringLiteral("id"), roleData(JobApplicationListModel::IdRole)},
		{QStringLiteral("companyId"), roleData(JobApplicationListModel::CompanyIdRole)},
		{QStringLiteral("companyName"), roleData(JobApplicationListModel::CompanyNameRole)},
		{QStringLiteral("companyInitials"), roleData(JobApplicationListModel::CompanyInitialsRole)},
		{QStringLiteral("companyAccent"), roleData(JobApplicationListModel::CompanyAccentRole)},
		{QStringLiteral("jobTitle"), roleData(JobApplicationListModel::JobTitleRole)},
		{QStringLiteral("jobUrl"), roleData(JobApplicationListModel::JobUrlRole)},
		{QStringLiteral("workFormat"), roleData(JobApplicationListModel::WorkFormatRole)},
		{QStringLiteral("city"), roleData(JobApplicationListModel::CityRole)},
		{QStringLiteral("salary"), roleData(JobApplicationListModel::SalaryRole)},
		{QStringLiteral("status"), roleData(JobApplicationListModel::StatusRole)},
		{QStringLiteral("statusLabel"), roleData(JobApplicationListModel::StatusLabelRole)},
		{QStringLiteral("statusAccent"), roleData(JobApplicationListModel::StatusAccentRole)},
		{QStringLiteral("appliedDate"), roleData(JobApplicationListModel::AppliedDateRole)},
		{QStringLiteral("dateLabel"), roleData(JobApplicationListModel::DateLabelRole)},
		{QStringLiteral("nextStep"), roleData(JobApplicationListModel::NextStepRole)},
		{QStringLiteral("cvId"), roleData(JobApplicationListModel::CvIdRole)},
		{QStringLiteral("cvFileName"), roleData(JobApplicationListModel::CvFileNameRole)},
		{QStringLiteral("description"), roleData(JobApplicationListModel::DescriptionRole)},
		{QStringLiteral("requirements"), roleData(JobApplicationListModel::RequirementsRole)},
		{QStringLiteral("techStack"), roleData(JobApplicationListModel::TechStackRole)},
		{QStringLiteral("notes"), roleData(JobApplicationListModel::NotesRole)},
	};
}

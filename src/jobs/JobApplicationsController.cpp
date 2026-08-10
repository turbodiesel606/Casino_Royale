#include "JobApplicationsController.hpp"
#include "AddJobWorker.hpp"
#include "JobApplicationValidator.hpp"
#include "common/CancellationState.hpp"

#include <memory>
#include <utility>

JobApplicationsController::JobApplicationsController(
	QVector<JobApplication> applications,
	AddJobWorker& addJobWorker,
	QObject* parent)
	: QObject(parent)
	, applicationsModel_(std::move(applications))
	, selectionTracker_(filteredApplicationsModel_, JobApplicationListModel::IdRole),
	addJobWorker_{ addJobWorker }
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
		&addJobWorker_,
		&AddJobWorker::admissionCompleted,
		this,
		&JobApplicationsController::handleAddJobAdmission);
	connect(
		&addJobWorker_,
		&AddJobWorker::saveCompleted,
		this,
		&JobApplicationsController::handleAddJobSave);
}

JobApplicationsController::~JobApplicationsController()
{
	shuttingDown_ = true;
	createQueue_.clear();
	if (activeCreateCancellation_ != nullptr) {
		activeCreateCancellation_->requestCancellation();
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
	return static_cast<int>(createQueue_.size())
		+ (activeCreateApplication_.has_value()
			? 1 : 0);
}

void JobApplicationsController::setSearchText(const QString& text)
{
	const auto normalized = text.trimmed();
	if (searchText_ == normalized) {
		return;
	}

	searchText_ = normalized;
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
	if (shuttingDown_) 
		return;

	const auto operationId = ++nextCreateOperationId_;
	const auto previousCount = pendingSaveCount();
	createQueue_.emplace_back(operationId, formValues, selectedCvUrl);
	publishPendingSaveStateChange(previousCount);
	emit applicationQueued(operationId);
	startNextCreateApplication();
}

void JobApplicationsController::startNextCreateApplication()
{
	if (shuttingDown_ || activeCreateApplication_.has_value() || createQueue_.empty())
		return;

	activeCreateApplication_ = std::move(createQueue_.front());
	createQueue_.pop_front();
	activeCreateCancellation_ = std::make_shared<CancellationState>();
	suppressActiveCompletionNotification_ = false;

	addJobWorker_.submit({
		activeCreateApplication_->operationId_,
		activeCreateApplication_->rawFormValues_,
		activeCreateApplication_->selectedCvUrl_,
		activeCreateCancellation_});
}

void JobApplicationsController::cancelCreateApplication()
{
	if (activeCreateCancellation_ != nullptr) {
		activeCreateCancellation_->requestCancellation();
	}
}

void JobApplicationsController::cancelAllCreateApplications()
{
	const auto previousCount = pendingSaveCount();
	createQueue_.clear();
	if (activeCreateCancellation_ != nullptr) {
		suppressActiveCompletionNotification_ = true;
		activeCreateCancellation_->requestCancellation();
	}
	publishPendingSaveStateChange(previousCount);
}

void JobApplicationsController::handleAddJobAdmission(const AddJobAdmissionOutcome& outcome)
{
	if (!isActiveCreateOutcome(outcome.operationId_, outcome.cancellation_)) {
		return;
	}

	if (outcome.isAccepted()) {
		if (!suppressActiveCompletionNotification_) {
			emit applicationAccepted(outcome.operationId_, outcome.jobTitle_);
		}
		return;
	}

	if (!suppressActiveCompletionNotification_) {
		emit applicationRejected(
			outcome.operationId_,
			outcome.fieldErrors_,
			outcome.message_);
	}
	releaseActiveCreateApplication();
}

void JobApplicationsController::handleAddJobSave(const AddJobSaveOutcome& outcome)
{
	if (!isActiveCreateOutcome(outcome.operationId_, outcome.cancellation_)) {
		return;
	}

	const auto& result = outcome.result_;

	if (result.success_) {
		applicationsModel_.appendApplication(result.application_);
		emit companyResolved(result.company_.id_, result.company_.name_);
		filteredApplicationsModel_.sort(filteredApplicationsModel_.sortColumn(), filteredApplicationsModel_.sortOrder());
		emit cvUsed(result.cvDocument_, result.application_.id_, result.cvWasInserted_);
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
	releaseActiveCreateApplication();
}

bool JobApplicationsController::isActiveCreateOutcome(
	quint64 operationId,
	const std::shared_ptr<CancellationState>& cancellation) const
{
	return !shuttingDown_
		&& activeCreateApplication_.has_value()
		&& activeCreateApplication_->operationId_ == operationId
		&& activeCreateCancellation_ == cancellation;
}

void JobApplicationsController::releaseActiveCreateApplication()
{
	const auto previousCount = pendingSaveCount();
	activeCreateApplication_.reset();
	activeCreateCancellation_.reset();
	suppressActiveCompletionNotification_ = false;
	publishPendingSaveStateChange(previousCount);
	startNextCreateApplication();
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

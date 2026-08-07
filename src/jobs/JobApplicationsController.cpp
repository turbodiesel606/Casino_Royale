#include "JobApplicationsController.hpp"
#include "AddJobService.hpp"
#include "JobApplicationDraft.hpp"
#include "JobApplicationValidator.hpp"

#include <QMetaObject>

#include <atomic>
#include <exception>
#include <memory>
#include <utility>
#include <thread>
#include <chrono>
namespace {
	JobApplicationDraft fill_draft(const QVariantMap& formValues) {
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
		draft.techStack_ = technologies.canConvert<QStringList>()
			? technologies.toStringList()
			: technologies.toString().split(',', Qt::SkipEmptyParts);

		return draft;
	}
}

JobApplicationsController::JobApplicationsController(
	QVector<JobApplication> applications,
	AddJobService& addJobService,
	QObject* parent)
	: QObject(parent)
	, applicationsModel_(std::move(applications))
	, selectionTracker_(filteredApplicationsModel_, JobApplicationListModel::IdRole),
	addJobService_{ addJobService }
{
	filePreparationPool_.setMaxThreadCount(1);
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
}

JobApplicationsController::~JobApplicationsController()
{
	shuttingDown_ = true;
	createQueue_.clear();
	if (activeCreateCancellation_ != nullptr) {
		activeCreateCancellation_->store(true, std::memory_order_relaxed);
	}
	filePreparationPool_.waitForDone();
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
	JobApplicationDraft draft = fill_draft(formValues);

	const auto preflight = addJobService_.preflight(draft, selectedCvUrl);

	if (!preflight.isValid()) {
		emit saveFailed(preflight.fieldErrors_, preflight.message_);
		return;
	}

	const auto previousCount = pendingSaveCount();
	const auto operationId = ++nextCreateOperationId_;
	createQueue_.emplace_back(operationId, preflight.draft_, selectedCvUrl);
	publishPendingSaveStateChange(previousCount);
	emit applicationQueued(operationId, preflight.draft_.jobTitle_);
	startNextCreateApplication();
	std::this_thread::sleep_for(std::chrono::seconds(5));// intended suppress
}

void JobApplicationsController::startNextCreateApplication()
{
	if (shuttingDown_ || activeCreateApplication_.has_value() || createQueue_.empty()) {
		return;
	}

	activeCreateApplication_ = std::move(createQueue_.front());
	createQueue_.pop_front();
	activeCreateCancellation_ = std::make_shared<std::atomic_bool>(false);
	suppressActiveCompletionNotification_ = false;

	const auto operationId = activeCreateApplication_->operationId_;
	const auto cancellation = activeCreateCancellation_;
	auto draft = activeCreateApplication_->draft_;
	const auto selectedCvUrl = activeCreateApplication_->selectedCvUrl_;

	filePreparationPool_.start([
		this,
		draft = std::move(draft),
		selectedCvUrl,
		cancellation,
		operationId]() mutable {
			AddJobPreparationResult preparation;

			try {
				preparation = addJobService_.prepare(draft, selectedCvUrl, cancellation);
			}
			catch (const std::exception& error) {
				preparation.message_ = QString::fromUtf8(error.what());
			}


			QMetaObject::invokeMethod(
				this,
				[this, operationId, cancellation, preparation = std::move(preparation)]() mutable {
				//std::this_thread::sleep_for(std::chrono::seconds(10));// intended suppress
					finishCreateApplication(operationId, cancellation, std::move(preparation));
				},
				Qt::QueuedConnection);
		});
}

void JobApplicationsController::cancelCreateApplication()
{
	if (activeCreateCancellation_ != nullptr) {
		activeCreateCancellation_->store(true, std::memory_order_relaxed);
	}
}

void JobApplicationsController::cancelAllCreateApplications()
{
	const auto previousCount = pendingSaveCount();
	createQueue_.clear();
	if (activeCreateCancellation_ != nullptr) {
		suppressActiveCompletionNotification_ = true;
		activeCreateCancellation_->store(true, std::memory_order_relaxed);
	}
	publishPendingSaveStateChange(previousCount);
}

void JobApplicationsController::finishCreateApplication(
	quint64 operationId,
	const std::shared_ptr<std::atomic_bool>& cancellation,
	AddJobPreparationResult preparation)
{
	if (shuttingDown_
		|| !activeCreateApplication_.has_value()
		|| activeCreateApplication_->operationId_ != operationId
		|| activeCreateCancellation_ != cancellation)
	{ return; }

	if (cancellation->load(std::memory_order_relaxed)) {
		preparation.success_ = false;
		preparation.cancelled_ = true;
		preparation.message_ = QStringLiteral("Job creation was canceled.");
	}

	const auto result = addJobService_.complete(std::move(preparation));
	const auto jobTitle = activeCreateApplication_->draft_.jobTitle_;
	const auto suppressCompletionNotification = suppressActiveCompletionNotification_;

	if (result.success_) {
		applicationsModel_.appendApplication(result.application_);
		emit companyResolved(result.company_.id_, result.company_.name_);
		filteredApplicationsModel_.sort(filteredApplicationsModel_.sortColumn(), filteredApplicationsModel_.sortOrder());
		emit cvUsed(result.cvDocument_, result.application_.id_, result.cvWasInserted_);
		emit applicationCreated(result.application_.id_);
	}

	if (!suppressCompletionNotification) {
		const auto message = result.message_.isEmpty()
			? (result.success_
				? QStringLiteral("Job application saved successfully.")
				: QStringLiteral("Job application could not be saved."))
			: result.message_;
		emit applicationSaveCompleted(operationId, jobTitle, result.success_, message);
	}

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
	if ((previousCount == 0) != (currentCount == 0)) {
		emit savingChanged();
	}
	if (previousCount > 0 && currentCount == 0 && !shuttingDown_) {
		emit saveQueueDrained();
	}
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

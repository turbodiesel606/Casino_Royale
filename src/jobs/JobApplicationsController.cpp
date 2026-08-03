#include "JobApplicationsController.hpp"
#include "AddJobService.hpp"
#include "JobApplicationDraft.hpp"
#include "JobApplicationFactory.hpp"
#include "JobApplicationValidator.hpp"

#include <QMetaObject>

#include <atomic>
#include <exception>
#include <memory>
#include <utility>


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
	if (createCancellation_ != nullptr) {
		createCancellation_->store(true, std::memory_order_relaxed);
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
	return saving_;
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
#include<iostream>

void JobApplicationsController::createApplication(
	const QVariantMap& formValues,
	const QUrl& selectedCvUrl)
{
	// Prevent overlapping requests because the controller tracks only one active save operation.
	if (saving_)
		return;

	// Translate the QML form map into the backend draft without applying business rules in the UI layer.
	// move in helper function
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

	// Accept the technology field as either a QML string list or a comma-separated string.
	const auto technologies = formValues.value(QStringLiteral("techStack"));
	draft.techStack_ = technologies.canConvert<QStringList>()
		? technologies.toStringList()
		: technologies.toString().split(',', Qt::SkipEmptyParts);
	
	// validate all fields
	const auto validation = JobApplicationValidator::validate(
		JobApplicationFactory::normalize(draft),
		selectedCvUrl);
	// search for empty or invalid jobTitle, jobUrl, cv fields
	QVariantMap preflightFieldErrors;
	for (const auto& fieldName : {
		QStringLiteral("jobTitle"),
		QStringLiteral("jobUrl"),
		QStringLiteral("cv")}) {

		// iter to either existing elem or end (if it didnt find)
		const auto error = validation.fieldErrors_.constFind(fieldName);
		
		if (error != validation.fieldErrors_.cend()) 
			preflightFieldErrors.insert(fieldName, error.value());
	}

	if (!preflightFieldErrors.isEmpty()) {
		emit saveFailed(
			preflightFieldErrors,
			QStringLiteral("Please correct the highlighted fields."));
		return;
	}

	// Publish the active-save state and create cancellation data shared with the worker task.
	saving_ = true;
	emit savingChanged();

	createCancellation_ = std::make_shared<std::atomic_bool>(false);
	const auto cancellation = createCancellation_;
	const auto operationId = ++createOperationId_;

	// Run validation, file hashing, and staging on the dedicated worker so the GUI thread remains responsive.
	filePreparationPool_.start([	// Maybe data race
		this,
		draft = std::move(draft),
		selectedCvUrl,
		cancellation,
		operationId]() mutable {
			AddJobPreparationResult preparation;

			// Convert unexpected worker failures into the result consumed by the QML-facing completion path.
			try {
				preparation = addJobService_.prepare(draft, selectedCvUrl, cancellation);
			}
			catch (const std::exception& error) {
				preparation.message_ = QString::fromUtf8(error.what());
			}

			// forward lambda to GUI thread and perform it in GUI thread
			QMetaObject::invokeMethod(
				this,
				[this, operationId, cancellation, preparation = std::move(preparation)]() mutable {
					finishCreateApplication(operationId, cancellation, std::move(preparation));
				},
				Qt::QueuedConnection);
		});
}

void JobApplicationsController::cancelCreateApplication()
{
	if (createCancellation_ != nullptr) 
		createCancellation_->store(true, std::memory_order_relaxed);
}

void JobApplicationsController::finishCreateApplication(
	quint64 operationId,
	const std::shared_ptr<std::atomic_bool>& cancellation,
	AddJobPreparationResult preparation)
{
	if (shuttingDown_ || operationId != createOperationId_) 
		return;
	
	// Maybe data race
	if (cancellation->load(std::memory_order_relaxed)) {
		preparation.success_ = false;
		preparation.cancelled_ = true;
		preparation.message_ = QStringLiteral("Job creation was canceled.");
	}
	const auto result = addJobService_.complete(std::move(preparation));
	createCancellation_.reset();
	saving_ = false;
	emit savingChanged();

	if (!result.success_) {
		emit saveFailed(result.fieldErrors_, result.message_);
		return;
	}

	applicationsModel_.appendApplication(result.application_);
	emit companyResolved(result.company_.id_, result.company_.name_);
	filteredApplicationsModel_.sort(filteredApplicationsModel_.sortColumn(), filteredApplicationsModel_.sortOrder());
	emit cvUsed(result.cvDocument_, result.application_.id_, result.cvWasInserted_);
	emit applicationCreated(result.application_.id_);
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

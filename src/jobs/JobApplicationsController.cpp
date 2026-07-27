#include "JobApplicationsController.hpp"

#include "AddJobService.hpp"
#include "JobApplicationDraft.hpp"
#include "common/ValidationService.hpp"

#include <utility>

JobApplicationsController::JobApplicationsController(QObject* parent)
    : JobApplicationsController(QVector<JobApplication>{}, parent)
{
}

JobApplicationsController::JobApplicationsController(QVector<JobApplication> applications, QObject* parent)
    : QObject(parent)
    , applicationsModel_(std::move(applications))
    , selectionTracker_(filteredApplicationsModel_, JobApplicationListModel::IdRole)
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
    filteredApplicationsModel_.setSort(JobApplicationListModel::DateLabelRole, Qt::DescendingOrder);
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

JobApplicationsController::JobApplicationsController(
    QVector<JobApplication> applications,
    AddJobService& addJobService,
    QObject* parent)
    : JobApplicationsController(std::move(applications), parent)
{
    addJobService_ = &addJobService;
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
    const auto* application = selectedSourceApplication();
    return application != nullptr ? applicationToMap(*application) : QVariantMap();
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
    } else {
        filteredApplicationsModel_.setExactFilter(JobApplicationListModel::StatusLabelRole, statusFilter_);
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

    const auto application = selectedApplication();
    auto result = ValidationService::validateRequiredFields(
        application,
        {QStringLiteral("jobTitle"), QStringLiteral("companyName"), QStringLiteral("status"), QStringLiteral("cvId")});
    const auto urlResult = ValidationService::validateHttpUrl(QStringLiteral("jobUrl"), application.value(QStringLiteral("jobUrl")).toString(), true);

    if (!urlResult.isValid_) {
        result.isValid_ = false;
        result.messages_.append(urlResult.messages_);
    }

    return result.messages_;
}

void JobApplicationsController::createApplication(
    const QVariantMap& formValues,
    const QUrl& selectedCvUrl)
{
    if (saving_) {
        return;
    }
    if (addJobService_ == nullptr) {
        emit saveFailed({}, QStringLiteral("Job storage is not available."));
        return;
    }

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

    saving_ = true;
    emit savingChanged();
    const auto result = addJobService_->create(draft, selectedCvUrl);
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

QVariantMap JobApplicationsController::applicationToMap(const JobApplication& application) const
{
    const auto sourceRow = selectionTracker_.selectedSourceIndex().row();
    return {
        {QStringLiteral("id"), application.id_},
        {QStringLiteral("companyId"), application.companyId_},
        {QStringLiteral("companyName"), application.companyName_},
        {QStringLiteral("companyInitials"), application.companyInitials_},
        {QStringLiteral("companyAccent"), application.companyAccent_},
        {QStringLiteral("jobTitle"), application.jobTitle_},
        {QStringLiteral("jobUrl"), application.jobUrl_},
        {QStringLiteral("workFormat"), application.workFormat_},
        {QStringLiteral("city"), application.city_},
        {QStringLiteral("salary"), application.salary_},
        {QStringLiteral("status"), application.status_},
        {QStringLiteral("statusLabel"), application.status_},
        {QStringLiteral("statusAccent"), sourceRow >= 0 ? applicationsModel_.data(applicationsModel_.index(sourceRow), JobApplicationListModel::StatusAccentRole) : QVariant()},
        {QStringLiteral("appliedDate"), application.appliedDate_},
        {QStringLiteral("dateLabel"), application.dateLabel_},
        {QStringLiteral("nextStep"), application.nextStep_},
        {QStringLiteral("cvId"), application.cvId_},
        {QStringLiteral("cvFileName"), application.cvFileName_},
        {QStringLiteral("description"), application.description_},
        {QStringLiteral("requirements"), application.requirements_},
        {QStringLiteral("techStack"), application.techStack_},
        {QStringLiteral("notes"), application.notes_},
    };
}

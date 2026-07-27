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
{
    filteredApplicationsModel_.setSourceModel(&applicationsModel_);
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
        &applicationsModel_,
        &QAbstractItemModel::rowsInserted,
        this,
        [this]() { refreshSelection(); });
    connect(
        &applicationsModel_,
        &QAbstractItemModel::rowsRemoved,
        this,
        [this]() { refreshSelection(); });
    connect(
        &applicationsModel_,
        &QAbstractItemModel::rowsMoved,
        this,
        [this]() { refreshSelection(); });
    connect(
        &applicationsModel_,
        &QAbstractItemModel::modelReset,
        this,
        [this]() { refreshSelection(!selectedApplicationId_.isEmpty()); });
    connect(
        &applicationsModel_,
        &QAbstractItemModel::layoutChanged,
        this,
        [this]() { refreshSelection(); });
    connect(
        &applicationsModel_,
        &QAbstractItemModel::dataChanged,
        this,
        [this](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
            const auto selectedRow = selectedSourceRow();
            refreshSelection(selectedRow >= topLeft.row() && selectedRow <= bottomRight.row());
        });
    refreshSelection();
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
    return selectedApplicationIndex_;
}

QString JobApplicationsController::selectedApplicationId() const
{
    return selectedApplicationId_;
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
    if (index < 0 || index >= filteredApplicationsModel_.rowCount()) {
        return;
    }

    const auto applicationId = filteredApplicationsModel_.data(
        filteredApplicationsModel_.index(index, 0),
        JobApplicationListModel::IdRole).toString();
    if (applicationId == selectedApplicationId_ && index == selectedApplicationIndex_) {
        return;
    }

    selectedApplicationId_ = applicationId;
    selectedApplicationIndex_ = index;
    emit selectedApplicationChanged();
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
    filteredApplicationsModel_.setSearchText(searchText_);
    refreshSelection();
    emit filtersChanged();
    emit applicationsModelChanged();
    emit resultSummaryChanged();
}

void JobApplicationsController::setStatusFilter(const QString& status)
{
    const auto normalized = status.trimmed();
    if (statusFilter_ == normalized) {
        return;
    }

    statusFilter_ = normalized;
    if (statusFilter_.isEmpty() || statusFilter_ == QStringLiteral("All")) {
        filteredApplicationsModel_.clearExactFilter();
    } else {
        filteredApplicationsModel_.setExactFilter(JobApplicationListModel::StatusLabelRole, statusFilter_);
    }
    refreshSelection();
    emit filtersChanged();
    emit applicationsModelChanged();
    emit resultSummaryChanged();
}

void JobApplicationsController::clearFilters()
{
    if (searchText_.isEmpty() && statusFilter_.isEmpty()) {
        return;
    }

    searchText_.clear();
    statusFilter_.clear();
    filteredApplicationsModel_.setSearchText(QString());
    filteredApplicationsModel_.clearExactFilter();
    refreshSelection();
    emit filtersChanged();
    emit applicationsModelChanged();
    emit resultSummaryChanged();
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
    refreshSelection();
    emit applicationsModelChanged();
    emit resultSummaryChanged();
    emit cvUsed(result.cvDocument_, result.application_.id_, result.cvWasInserted_);
    emit applicationCreated(result.application_.id_);
}

const JobApplication* JobApplicationsController::selectedSourceApplication() const
{
    const auto sourceRow = selectedSourceRow();
    return sourceRow >= 0 ? applicationsModel_.applicationAt(sourceRow) : nullptr;
}

int JobApplicationsController::selectedSourceRow() const
{
    if (selectedApplicationId_.isEmpty()) {
        return -1;
    }

    for (int row = 0; row < applicationsModel_.rowCount(); ++row) {
        const auto* application = applicationsModel_.applicationAt(row);
        if (application != nullptr && application->id_ == selectedApplicationId_) {
            return row;
        }
    }

    return -1;
}

void JobApplicationsController::refreshSelection(bool selectedDataChanged)
{
    const auto previousId = selectedApplicationId_;
    const auto previousIndex = selectedApplicationIndex_;
    auto nextIndex = -1;

    if (!selectedApplicationId_.isEmpty()) {
        for (int row = 0; row < filteredApplicationsModel_.rowCount(); ++row) {
            if (filteredApplicationsModel_.data(
                    filteredApplicationsModel_.index(row, 0),
                    JobApplicationListModel::IdRole).toString() == selectedApplicationId_) {
                nextIndex = row;
                break;
            }
        }
    }

    if (nextIndex < 0) {
        if (filteredApplicationsModel_.rowCount() > 0) {
            nextIndex = 0;
            selectedApplicationId_ = filteredApplicationsModel_.data(
                filteredApplicationsModel_.index(0, 0),
                JobApplicationListModel::IdRole).toString();
        } else {
            selectedApplicationId_.clear();
        }
    }

    selectedApplicationIndex_ = nextIndex;
    if (selectedApplicationId_ != previousId
        || selectedApplicationIndex_ != previousIndex
        || selectedDataChanged) {
        emit selectedApplicationChanged();
    }
}

QVariantMap JobApplicationsController::applicationToMap(const JobApplication& application) const
{
    const auto sourceRow = selectedSourceRow();
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

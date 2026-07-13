#include "JobApplicationsController.hpp"

#include "AddJobService.hpp"
#include "JobApplicationDraft.hpp"
#include "common/ValidationService.hpp"

#include <algorithm>
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
    selectedApplicationIndex_ = filteredApplicationsModel_.rowCount() > 0 ? 0 : -1;
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
    const auto* application = selectedSourceApplication();
    return application != nullptr ? application->id_ : QString();
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
    if (index == selectedApplicationIndex_ || index < 0 || index >= filteredApplicationsModel_.rowCount()) {
        return;
    }

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
    refreshSelectionAfterFilterChange();
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
    refreshSelectionAfterFilterChange();
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
    refreshSelectionAfterFilterChange();
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
    filteredApplicationsModel_.sort(filteredApplicationsModel_.sortColumn(), filteredApplicationsModel_.sortOrder());
    refreshSelectionAfterFilterChange();
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
    const auto proxyIndex = filteredApplicationsModel_.index(selectedApplicationIndex_, 0);
    if (!proxyIndex.isValid()) {
        return -1;
    }

    return filteredApplicationsModel_.mapToSource(proxyIndex).row();
}

void JobApplicationsController::refreshSelectionAfterFilterChange()
{
    const auto previousIndex = selectedApplicationIndex_;
    const auto rowCount = filteredApplicationsModel_.rowCount();
    selectedApplicationIndex_ = rowCount > 0 ? std::clamp(selectedApplicationIndex_, 0, rowCount - 1) : -1;
    if (selectedApplicationIndex_ != previousIndex || selectedApplicationIndex_ >= 0) {
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

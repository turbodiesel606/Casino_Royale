#include "JobApplicationsController.h"

#include "common/ValidationService.h"

#include <algorithm>

JobApplicationsController::JobApplicationsController(QObject* parent)
    : QObject(parent)
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

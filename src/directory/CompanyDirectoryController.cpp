#include "CompanyDirectoryController.h"

#include <algorithm>

CompanyDirectoryController::CompanyDirectoryController(const JobApplicationListModel& applicationsModel, const ContactListModel& contactModel, QObject* parent)
    : QObject(parent)
    , companyModel_(this)
    , filteredCompanyModel_(this)
    , linkedJobsModel_(applicationsModel, this)
    , linkedContactsModel_(contactModel, this)
{
    filteredCompanyModel_.setSourceModel(&companyModel_);
    filteredCompanyModel_.setSearchRoles({
        CompanyListModel::NameRole,
        CompanyListModel::WebsiteRole,
        CompanyListModel::DescriptionRole,
        CompanyListModel::NotesRole,
    });
    updateLinkedModels();
}

QAbstractItemModel* CompanyDirectoryController::companyModel()
{
    return &filteredCompanyModel_;
}

QAbstractItemModel* CompanyDirectoryController::linkedJobsModel()
{
    return &linkedJobsModel_;
}

QAbstractItemModel* CompanyDirectoryController::linkedContactsModel()
{
    return &linkedContactsModel_;
}

int CompanyDirectoryController::companyCount() const
{
    return filteredCompanyModel_.rowCount();
}

int CompanyDirectoryController::selectedCompanyIndex() const
{
    return selectedCompanyIndex_;
}

QString CompanyDirectoryController::selectedCompanyId() const
{
    const auto* company = selectedSourceCompany();
    return company != nullptr ? company->id_ : QString();
}

QVariantMap CompanyDirectoryController::selectedCompany() const
{
    const auto* company = selectedSourceCompany();
    return company != nullptr ? companyToMap(*company) : QVariantMap();
}

QString CompanyDirectoryController::searchText() const
{
    return searchText_;
}

QString CompanyDirectoryController::sortMode() const
{
    return sortMode_;
}

QString CompanyDirectoryController::resultSummary() const
{
    const auto count = companyCount();
    if (count == 0) {
        return QStringLiteral("Showing 0 companies");
    }
    return count == 1 ? QStringLiteral("Showing 1 company") : QStringLiteral("Showing 1 to %1 of %1 companies").arg(count);
}

void CompanyDirectoryController::selectCompany(int index)
{
    if (index == selectedCompanyIndex_ || index < 0 || index >= filteredCompanyModel_.rowCount()) {
        return;
    }

    selectedCompanyIndex_ = index;
    updateLinkedModels();
    emit selectedCompanyChanged();
    emit linkedModelsChanged();
}

void CompanyDirectoryController::setSearchText(const QString& text)
{
    const auto normalized = text.trimmed();
    if (searchText_ == normalized) {
        return;
    }

    searchText_ = normalized;
    filteredCompanyModel_.setSearchText(searchText_);
    refreshSelectionAfterFilterChange();
    emit filtersChanged();
    emit companyModelChanged();
    emit resultSummaryChanged();
}

void CompanyDirectoryController::setSortMode(const QString& sortMode)
{
    const auto normalized = sortMode.trimmed().isEmpty() ? QStringLiteral("Name") : sortMode.trimmed();
    if (sortMode_ == normalized) {
        return;
    }

    sortMode_ = normalized;
    if (sortMode_ == QStringLiteral("Open Jobs")) {
        filteredCompanyModel_.setSort(CompanyListModel::OpenJobCountRole, Qt::DescendingOrder);
    } else if (sortMode_ == QStringLiteral("Contacts")) {
        filteredCompanyModel_.setSort(CompanyListModel::ContactCountRole, Qt::DescendingOrder);
    } else {
        filteredCompanyModel_.setSort(CompanyListModel::NameRole, Qt::AscendingOrder);
    }
    refreshSelectionAfterFilterChange();
    emit filtersChanged();
    emit companyModelChanged();
}

void CompanyDirectoryController::clearFilters()
{
    if (searchText_.isEmpty()) {
        return;
    }

    searchText_.clear();
    filteredCompanyModel_.setSearchText(QString());
    refreshSelectionAfterFilterChange();
    emit filtersChanged();
    emit companyModelChanged();
    emit resultSummaryChanged();
}

const Company* CompanyDirectoryController::selectedSourceCompany() const
{
    const auto sourceRow = selectedSourceRow();
    return sourceRow >= 0 ? companyModel_.companyAt(sourceRow) : nullptr;
}

int CompanyDirectoryController::selectedSourceRow() const
{
    const auto proxyIndex = filteredCompanyModel_.index(selectedCompanyIndex_, 0);
    if (!proxyIndex.isValid()) {
        return -1;
    }

    return filteredCompanyModel_.mapToSource(proxyIndex).row();
}

void CompanyDirectoryController::refreshSelectionAfterFilterChange()
{
    const auto previousIndex = selectedCompanyIndex_;
    const auto rowCount = filteredCompanyModel_.rowCount();
    selectedCompanyIndex_ = rowCount > 0 ? std::clamp(selectedCompanyIndex_, 0, rowCount - 1) : -1;
    updateLinkedModels();
    if (selectedCompanyIndex_ != previousIndex || selectedCompanyIndex_ >= 0) {
        emit selectedCompanyChanged();
        emit linkedModelsChanged();
    }
}

QVariantMap CompanyDirectoryController::companyToMap(const Company& company) const
{
    return {
        {QStringLiteral("id"), company.id_},
        {QStringLiteral("name"), company.name_},
        {QStringLiteral("website"), company.website_},
        {QStringLiteral("logoText"), company.logoText_},
        {QStringLiteral("logoAccent"), company.logoAccent_},
        {QStringLiteral("openJobCount"), company.openJobCount_},
        {QStringLiteral("contactCount"), company.contactCount_},
        {QStringLiteral("lastActivityLabel"), company.lastActivityLabel_},
        {QStringLiteral("description"), company.description_},
        {QStringLiteral("notes"), company.notes_},
    };
}

void CompanyDirectoryController::updateLinkedModels()
{
    linkedJobsModel_.setCompanyId(selectedCompanyId());
    linkedContactsModel_.setCompanyId(selectedCompanyId());
}

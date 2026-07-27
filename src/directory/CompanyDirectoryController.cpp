#include "CompanyDirectoryController.hpp"

#include "ContactListModel.hpp"
#include "jobs/JobApplicationListModel.hpp"

#include <QHash>
#include <utility>

CompanyDirectoryController::CompanyDirectoryController(const JobApplicationListModel& applicationsModel, const ContactListModel& contactModel, QObject* parent)
    : CompanyDirectoryController(QVector<Company>{}, applicationsModel, contactModel, parent)
{
}

CompanyDirectoryController::CompanyDirectoryController(
    QVector<Company> companies,
    const JobApplicationListModel& applicationsModel,
    const ContactListModel& contactModel,
    QObject* parent)
    : QObject(parent)
    , applicationsModel_(applicationsModel)
    , companyModel_(std::move(companies), this)
    , filteredCompanyModel_(this)
    , linkedJobsModel_(
        applicationsModel,
        JobApplicationListModel::CompanyIdRole,
        this)
    , linkedContactsModel_(
        contactModel,
        ContactListModel::CompanyIdRole,
        this)
    , selectionTracker_(filteredCompanyModel_, CompanyListModel::IdRole)
{
    filteredCompanyModel_.setSearchRoles({
        CompanyListModel::NameRole,
        CompanyListModel::WebsiteRole,
        CompanyListModel::DescriptionRole,
        CompanyListModel::NotesRole,
    });
    QObject::connect(
        &applicationsModel_,
        &QAbstractItemModel::rowsInserted,
        this,
        [this]() { refreshCompanyJobCounts(); });
    QObject::connect(
        &applicationsModel_,
        &QAbstractItemModel::modelReset,
        this,
        [this]() { refreshCompanyJobCounts(); });
    refreshCompanyJobCounts();
    connect(
        &selectionTracker_,
        &StableIdSelectionTracker::selectionChanged,
        this,
        &CompanyDirectoryController::handleSelectionChanged);
    filteredCompanyModel_.setSourceModel(&companyModel_);
    selectionTracker_.synchronize();
    publishedCompanyCount_ = companyCount();
    connect(
        &filteredCompanyModel_,
        &QAbstractItemModel::rowsInserted,
        this,
        [this]() { handleVisibleCountChanged(); });
    connect(
        &filteredCompanyModel_,
        &QAbstractItemModel::rowsRemoved,
        this,
        [this]() { handleVisibleCountChanged(); });
    connect(
        &filteredCompanyModel_,
        &QAbstractItemModel::modelReset,
        this,
        [this]() { handleVisibleCountChanged(); });
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
    return selectionTracker_.selectedRow();
}

QString CompanyDirectoryController::selectedCompanyId() const
{
    return selectionTracker_.selectedId();
}

QVariantMap CompanyDirectoryController::selectedCompany() const
{
    const auto sourceIndex = selectionTracker_.selectedSourceIndex();
    return sourceIndex.isValid() ? companyToMap(sourceIndex.row()) : QVariantMap();
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
    selectionTracker_.selectRow(index);
}

void CompanyDirectoryController::setSearchText(const QString& text)
{
    const auto normalized = text.trimmed();
    if (searchText_ == normalized) {
        return;
    }

    searchText_ = normalized;
    selectionTracker_.beginModelUpdate();
    visibleCountNotificationsSuppressed_ = true;
    filteredCompanyModel_.setSearchText(searchText_);
    visibleCountNotificationsSuppressed_ = false;
    selectionTracker_.endModelUpdate();
    handleVisibleCountChanged();
    emit searchTextChanged();
}

void CompanyDirectoryController::setSortMode(const QString& sortMode)
{
    const auto normalized = sortMode.trimmed().isEmpty() ? QStringLiteral("Name") : sortMode.trimmed();
    if (sortMode_ == normalized) {
        return;
    }

    sortMode_ = normalized;
    selectionTracker_.beginModelUpdate();
    if (sortMode_ == QStringLiteral("Open Jobs")) {
        filteredCompanyModel_.setSort(CompanyListModel::OpenJobCountRole, Qt::DescendingOrder);
    } else if (sortMode_ == QStringLiteral("Contacts")) {
        filteredCompanyModel_.setSort(CompanyListModel::ContactCountRole, Qt::DescendingOrder);
    } else {
        filteredCompanyModel_.setSort(CompanyListModel::NameRole, Qt::AscendingOrder);
    }
    selectionTracker_.endModelUpdate();
    emit sortModeChanged();
}

void CompanyDirectoryController::clearFilters()
{
    if (searchText_.isEmpty()) {
        return;
    }

    searchText_.clear();
    selectionTracker_.beginModelUpdate();
    visibleCountNotificationsSuppressed_ = true;
    filteredCompanyModel_.setSearchText(QString());
    visibleCountNotificationsSuppressed_ = false;
    selectionTracker_.endModelUpdate();
    handleVisibleCountChanged();
    emit searchTextChanged();
}

void CompanyDirectoryController::publishCompany(
    const QString& companyId,
    const QString& companyName)
{
    const auto displayName = companyName.trimmed();
    if (companyId.isEmpty() || displayName.isEmpty()) {
        return;
    }

    Company company;
    company.id_ = companyId;
    company.name_ = displayName;
    companyModel_.upsertCompany(std::move(company));
    refreshCompanyJobCounts();
}

const Company* CompanyDirectoryController::selectedSourceCompany() const
{
    const auto sourceIndex = selectionTracker_.selectedSourceIndex();
    return sourceIndex.isValid() ? companyModel_.companyAt(sourceIndex.row()) : nullptr;
}

void CompanyDirectoryController::handleSelectionChanged(
    bool idChanged,
    bool rowChanged,
    bool dataChanged)
{
    if (idChanged) {
        updateLinkedModels();
        emit selectedCompanyIdChanged();
    }
    if (rowChanged) {
        emit selectedCompanyIndexChanged();
    }
    if (idChanged || dataChanged) {
        emit selectedCompanyChanged();
    }
}

void CompanyDirectoryController::handleVisibleCountChanged()
{
    if (visibleCountNotificationsSuppressed_) {
        return;
    }

    const auto count = companyCount();
    if (publishedCompanyCount_ == count) {
        return;
    }

    publishedCompanyCount_ = count;
    emit companyCountChanged();
    emit resultSummaryChanged();
}

void CompanyDirectoryController::refreshCompanyJobCounts()
{
    QHash<QString, int> jobCounts;
    for (int row = 0; row < applicationsModel_.rowCount(); ++row) {
        const auto index = applicationsModel_.index(row, 0);
        const auto companyId = applicationsModel_.data(
            index,
            JobApplicationListModel::CompanyIdRole).toString();
        if (!companyId.isEmpty()) {
            ++jobCounts[companyId];
        }
    }

    for (int row = 0; row < companyModel_.rowCount(); ++row) {
        const auto* company = companyModel_.companyAt(row);
        if (company != nullptr) {
            companyModel_.setOpenJobCount(company->id_, jobCounts.value(company->id_));
        }
    }
}

QVariantMap CompanyDirectoryController::companyToMap(int sourceRow) const
{
    const auto modelIndex = companyModel_.index(sourceRow, 0);
    const auto roleData = [this, &modelIndex](int role) {
        return companyModel_.data(modelIndex, role);
    };
    return {
        {QStringLiteral("id"), roleData(CompanyListModel::IdRole)},
        {QStringLiteral("name"), roleData(CompanyListModel::NameRole)},
        {QStringLiteral("website"), roleData(CompanyListModel::WebsiteRole)},
        {QStringLiteral("logoText"), roleData(CompanyListModel::LogoTextRole)},
        {QStringLiteral("logoAccent"), roleData(CompanyListModel::LogoAccentRole)},
        {QStringLiteral("openJobCount"), roleData(CompanyListModel::OpenJobCountRole)},
        {QStringLiteral("contactCount"), roleData(CompanyListModel::ContactCountRole)},
        {QStringLiteral("lastActivityLabel"), roleData(CompanyListModel::LastActivityLabelRole)},
        {QStringLiteral("description"), roleData(CompanyListModel::DescriptionRole)},
        {QStringLiteral("notes"), roleData(CompanyListModel::NotesRole)},
    };
}

void CompanyDirectoryController::updateLinkedModels()
{
    linkedJobsModel_.setSelectedId(selectedCompanyId());
    linkedContactsModel_.setSelectedId(selectedCompanyId());
}

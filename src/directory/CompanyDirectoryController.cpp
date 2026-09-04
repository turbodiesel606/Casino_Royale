#include "CompanyDirectoryController.hpp"

#include "ContactListModel.hpp"
#include "common/ModelRoleUtils.hpp"
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
        &QAbstractItemModel::rowsRemoved,
        this,
        [this]() { refreshCompanyJobCounts(); });
    QObject::connect(
        &applicationsModel_,
        &QAbstractItemModel::modelReset,
        this,
        [this]() { refreshCompanyJobCounts(); });
    QObject::connect(
        &applicationsModel_,
        &QAbstractItemModel::dataChanged,
        this,
        [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
            if (roles.isEmpty()
                || roles.contains(JobApplicationListModel::CompanyIdRole)) {
                refreshCompanyJobCounts();
            }
        });
    refreshCompanyJobCounts();
    connect(
        &selectionTracker_,
        &StableIdSelectionTracker::selectionChanged,
        this,
        &CompanyDirectoryController::handleSelectionChanged);
    connect(
        &selectionTracker_,
        &StableIdSelectionTracker::visibleRowCountChanged,
        this,
        [this]() {
            emit companyCountChanged();
            emit resultSummaryChanged();
        });
    filteredCompanyModel_.setSourceModel(&companyModel_);
    selectionTracker_.synchronize();
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
    return selectionTracker_.visibleRowCount();
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
    return filteredCompanyModel_.searchText();
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
    if (searchText() == normalized) {
        return;
    }

    selectionTracker_.beginModelUpdate();
    filteredCompanyModel_.setSearchText(normalized);
    selectionTracker_.endModelUpdate();
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
    if (searchText().isEmpty()) {
        return;
    }

    selectionTracker_.beginModelUpdate();
    filteredCompanyModel_.setSearchText(QString());
    selectionTracker_.endModelUpdate();
    emit searchTextChanged();
}

void CompanyDirectoryController::publishCompany(const Company& company)
{
    auto publishedCompany = company;
    publishedCompany.name_ = publishedCompany.name_.trimmed();
    if (publishedCompany.id_.isEmpty() || publishedCompany.name_.isEmpty()) {
        return;
    }

    companyModel_.upsertCompany(std::move(publishedCompany));
    refreshCompanyJobCounts();
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
    return common::model::rowToVariantMap(
        companyModel_,
        sourceRow,
        {
            CompanyListModel::IdRole,
            CompanyListModel::NameRole,
            CompanyListModel::WebsiteRole,
            CompanyListModel::LogoTextRole,
            CompanyListModel::LogoAccentRole,
            CompanyListModel::OpenJobCountRole,
            CompanyListModel::ContactCountRole,
            CompanyListModel::LastActivityLabelRole,
            CompanyListModel::DescriptionRole,
            CompanyListModel::NotesRole,
        });
}

void CompanyDirectoryController::updateLinkedModels()
{
    linkedJobsModel_.setSelectedId(selectedCompanyId());
    linkedContactsModel_.setSelectedId(selectedCompanyId());
}

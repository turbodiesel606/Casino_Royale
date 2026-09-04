#include "CompanyListModel.hpp"

#include "common/ModelPresentation.hpp"

#include <utility>

namespace {

QVariant roleValue(const Company& company, int role)
{
    switch (role) {
    case CompanyListModel::IdRole:
        return company.id_;
    case CompanyListModel::NameRole:
        return company.name_;
    case CompanyListModel::WebsiteRole:
        return company.website_.toString();
    case CompanyListModel::LogoTextRole:
        return common::presentation::twoCharacterInitials(company.name_);
    case CompanyListModel::LogoAccentRole:
        return common::presentation::companyAccent();
    case CompanyListModel::OpenJobCountRole:
        return company.openJobCount_;
    case CompanyListModel::OpenJobCountLabelRole:
        return common::presentation::countLabel(
            company.openJobCount_,
            QStringLiteral("job"),
            QStringLiteral("jobs"));
    case CompanyListModel::ContactCountRole:
        return company.contactCount_;
    case CompanyListModel::ContactCountLabelRole:
        return common::presentation::countLabel(
            company.contactCount_,
            QStringLiteral("contact"),
            QStringLiteral("contacts"));
    case CompanyListModel::LastActivityLabelRole:
        return common::presentation::shortLocalDateLabel(company.lastActivityAt_);
    case CompanyListModel::DescriptionRole:
        return company.description_;
    case CompanyListModel::NotesRole:
        return company.notes_;
    case CompanyListModel::CreatedAtRole:
        return company.createdAt_;
    case CompanyListModel::UpdatedAtRole:
        return company.updatedAt_;
    default:
        return {};
    }
}

}

CompanyListModel::CompanyListModel(QObject* parent)
    : CompanyListModel(QVector<Company>{}, parent)
{
}

CompanyListModel::CompanyListModel(QVector<Company> companies, QObject* parent)
    : QAbstractListModel(parent)
    , companies_(std::move(companies))
{
}

int CompanyListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : companies_.size();
}

QVariant CompanyListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= companies_.size()) {
        return {};
    }
    return roleValue(companies_.at(index.row()), role);
}

QHash<int, QByteArray> CompanyListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {WebsiteRole, "website"},
        {LogoTextRole, "logoText"},
        {LogoAccentRole, "logoAccent"},
        {OpenJobCountRole, "openJobCount"},
        {OpenJobCountLabelRole, "openJobCountLabel"},
        {ContactCountRole, "contactCount"},
        {ContactCountLabelRole, "contactCountLabel"},
        {LastActivityLabelRole, "lastActivityLabel"},
        {DescriptionRole, "description"},
        {NotesRole, "notes"},
        {CreatedAtRole, "createdAt"},
        {UpdatedAtRole, "updatedAt"},
    };
}

const Company* CompanyListModel::companyAt(int row) const
{
    return row >= 0 && row < companies_.size() ? &companies_.at(row) : nullptr;
}

int CompanyListModel::rowForId(const QString& companyId) const
{
    if (companyId.isEmpty()) {
        return -1;
    }

    for (int row = 0; row < companies_.size(); ++row) {
        if (companies_.at(row).id_ == companyId) {
            return row;
        }
    }
    return -1;
}

bool CompanyListModel::upsertCompany(Company company)
{
    if (company.id_.isEmpty() || rowForId(company.id_) >= 0) {
        return false;
    }

    const auto row = companies_.size();
    beginInsertRows({}, row, row);
    companies_.append(std::move(company));
    endInsertRows();
    return true;
}

void CompanyListModel::setOpenJobCount(const QString& companyId, int count)
{
    const auto row = rowForId(companyId);
    if (row < 0 || companies_[row].openJobCount_ == count) {
        return;
    }

    companies_[row].openJobCount_ = count;
    const auto modelIndex = index(row, 0);
    emit dataChanged(
        modelIndex,
        modelIndex,
        {OpenJobCountRole, OpenJobCountLabelRole});
}

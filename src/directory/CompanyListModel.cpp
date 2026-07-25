#include "CompanyListModel.hpp"

#include <utility>

namespace {

QString countLabel(int count, const QString& singular, const QString& plural)
{
    return count == 1 ? QStringLiteral("1 %1").arg(singular) : QStringLiteral("%1 %2").arg(count).arg(plural);
}

QVariant roleValue(const Company& company, int role)
{
    switch (role) {
    case CompanyListModel::IdRole:
        return company.id_;
    case CompanyListModel::NameRole:
        return company.name_;
    case CompanyListModel::WebsiteRole:
        return company.website_;
    case CompanyListModel::LogoTextRole:
        return company.logoText_;
    case CompanyListModel::LogoAccentRole:
        return company.logoAccent_;
    case CompanyListModel::OpenJobCountRole:
        return company.openJobCount_;
    case CompanyListModel::OpenJobCountLabelRole:
        return countLabel(company.openJobCount_, QStringLiteral("job"), QStringLiteral("jobs"));
    case CompanyListModel::ContactCountRole:
        return company.contactCount_;
    case CompanyListModel::ContactCountLabelRole:
        return countLabel(company.contactCount_, QStringLiteral("contact"), QStringLiteral("contacts"));
    case CompanyListModel::LastActivityLabelRole:
        return company.lastActivityLabel_;
    case CompanyListModel::DescriptionRole:
        return company.description_;
    case CompanyListModel::NotesRole:
        return company.notes_;
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
    };
}

const Company* CompanyListModel::companyAt(int row) const
{
    return row >= 0 && row < companies_.size() ? &companies_.at(row) : nullptr;
}

bool CompanyListModel::upsertCompany(Company company)
{
    if (company.id_.isEmpty() || indexOfCompany(company.id_) >= 0) {
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
    const auto row = indexOfCompany(companyId);
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

int CompanyListModel::indexOfCompany(const QString& companyId) const
{
    for (int row = 0; row < companies_.size(); ++row) {
        if (companies_.at(row).id_ == companyId) {
            return row;
        }
    }
    return -1;
}

#include "CompanyListModel.hpp"

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
    : QAbstractListModel(parent)
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

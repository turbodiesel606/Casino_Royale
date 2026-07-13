#include "LinkedCompanyContactsModel.hpp"

LinkedCompanyContactsModel::LinkedCompanyContactsModel(const ContactListModel& contactModel, QObject* parent)
    : QAbstractListModel(parent)
    , contactModel_(contactModel)
{
}

int LinkedCompanyContactsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return sourceRows_.size();
}

QVariant LinkedCompanyContactsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= sourceRows_.size()) {
        return {};
    }

    const auto sourceRow = sourceRows_.at(index.row());
    switch (role) {
    case IdRole:
        return sourceData(sourceRow, ContactListModel::IdRole);
    case DisplayNameRole:
        return sourceData(sourceRow, ContactListModel::DisplayNameRole);
    case InitialsRole:
        return sourceData(sourceRow, ContactListModel::InitialsRole);
    case AvatarAccentRole:
        return sourceData(sourceRow, ContactListModel::AvatarAccentRole);
    case RoleTitleRole:
        return sourceData(sourceRow, ContactListModel::RoleTitleRole);
    case EmailRole:
        return sourceData(sourceRow, ContactListModel::EmailRole);
    case TelegramRole:
        return sourceData(sourceRow, ContactListModel::TelegramRole);
    case LinkedinRole:
        return sourceData(sourceRow, ContactListModel::LinkedinRole);
    case LastContactLabelRole:
        return sourceData(sourceRow, ContactListModel::LastContactLabelRole);
    default:
        return {};
    }
}

QHash<int, QByteArray> LinkedCompanyContactsModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {DisplayNameRole, "displayName"},
        {InitialsRole, "initials"},
        {AvatarAccentRole, "avatarAccent"},
        {RoleTitleRole, "roleTitle"},
        {EmailRole, "email"},
        {TelegramRole, "telegram"},
        {LinkedinRole, "linkedin"},
        {LastContactLabelRole, "lastContactLabel"},
    };
}

void LinkedCompanyContactsModel::setCompanyId(const QString& companyId)
{
    if (companyId_ == companyId) {
        return;
    }

    beginResetModel();
    companyId_ = companyId;
    rebuildSourceRows();
    endResetModel();
}

QVariant LinkedCompanyContactsModel::sourceData(int sourceRow, int role) const
{
    return contactModel_.data(contactModel_.index(sourceRow, 0), role);
}

void LinkedCompanyContactsModel::rebuildSourceRows()
{
    sourceRows_.clear();

    for (int row = 0; row < contactModel_.rowCount(); ++row) {
        if (sourceData(row, ContactListModel::CompanyIdRole).toString() == companyId_) {
            sourceRows_.append(row);
        }
    }
}

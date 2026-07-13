#include "ContactListModel.hpp"

namespace {

QVariant roleValue(const Contact& contact, int role)
{
    switch (role) {
    case ContactListModel::IdRole:
        return contact.id_;
    case ContactListModel::DisplayNameRole:
        return contact.displayName_;
    case ContactListModel::InitialsRole:
        return contact.initials_;
    case ContactListModel::AvatarAccentRole:
        return contact.avatarAccent_;
    case ContactListModel::RoleTitleRole:
        return contact.roleTitle_;
    case ContactListModel::CompanyIdRole:
        return contact.companyId_;
    case ContactListModel::CompanyNameRole:
        return contact.companyName_;
    case ContactListModel::RelatedApplicationIdRole:
        return contact.relatedApplicationId_;
    case ContactListModel::RelatedApplicationTitleRole:
        return contact.relatedApplicationTitle_;
    case ContactListModel::EmailRole:
        return contact.email_;
    case ContactListModel::TelegramRole:
        return contact.telegram_;
    case ContactListModel::LinkedinRole:
        return contact.linkedin_;
    case ContactListModel::LastContactLabelRole:
        return contact.lastContactLabel_;
    case ContactListModel::NotesRole:
        return contact.notes_;
    default:
        return {};
    }
}

}

ContactListModel::ContactListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ContactListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : contacts_.size();
}

QVariant ContactListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= contacts_.size()) {
        return {};
    }
    return roleValue(contacts_.at(index.row()), role);
}

QHash<int, QByteArray> ContactListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {DisplayNameRole, "displayName"},
        {InitialsRole, "initials"},
        {AvatarAccentRole, "avatarAccent"},
        {RoleTitleRole, "roleTitle"},
        {CompanyIdRole, "companyId"},
        {CompanyNameRole, "companyName"},
        {RelatedApplicationIdRole, "relatedApplicationId"},
        {RelatedApplicationTitleRole, "relatedApplicationTitle"},
        {EmailRole, "email"},
        {TelegramRole, "telegram"},
        {LinkedinRole, "linkedin"},
        {LastContactLabelRole, "lastContactLabel"},
        {NotesRole, "notes"},
    };
}

const Contact* ContactListModel::contactAt(int row) const
{
    return row >= 0 && row < contacts_.size() ? &contacts_.at(row) : nullptr;
}

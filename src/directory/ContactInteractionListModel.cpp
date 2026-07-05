#include "ContactInteractionListModel.h"

#include <utility>

ContactInteractionListModel::ContactInteractionListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ContactInteractionListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return interactions_.size();
}

QVariant ContactInteractionListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= interactions_.size()) {
        return {};
    }

    const auto& interaction = interactions_.at(index.row());
    switch (role) {
    case TypeRole:
        return interaction.type_;
    case TitleRole:
        return interaction.title_;
    case TimestampLabelRole:
        return interaction.timestampLabel_;
    case NotesRole:
        return interaction.notes_;
    default:
        return {};
    }
}

QHash<int, QByteArray> ContactInteractionListModel::roleNames() const
{
    return {
        {TypeRole, "type"},
        {TitleRole, "title"},
        {TimestampLabelRole, "timestampLabel"},
        {NotesRole, "notes"},
    };
}

void ContactInteractionListModel::setInteractions(QVector<ContactInteraction> interactions)
{
    beginResetModel();
    interactions_ = std::move(interactions);
    endResetModel();
}

#include "DashboardRecentCvsModel.h"

#include <algorithm>

namespace {

constexpr int recentCvLimit = 5;

}

DashboardRecentCvsModel::DashboardRecentCvsModel(const CvListModel& sourceModel, QObject* parent)
    : QAbstractListModel(parent)
    , sourceModel_(sourceModel)
{
}

int DashboardRecentCvsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : std::min(recentCvLimit, sourceModel_.rowCount());
}

QVariant DashboardRecentCvsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const auto sourceIndex = sourceModel_.index(index.row(), 0);
    switch (role) {
    case IdRole:
        return sourceModel_.data(sourceIndex, CvListModel::IdRole);
    case FileNameRole:
        return sourceModel_.data(sourceIndex, CvListModel::FileNameRole);
    case CategoryRole:
        return sourceModel_.data(sourceIndex, CvListModel::CategoryRole);
    case CategoryAccentRole:
        return sourceModel_.data(sourceIndex, CvListModel::CategoryAccentRole);
    case LanguageRole:
        return sourceModel_.data(sourceIndex, CvListModel::LanguageRole);
    case LanguageAccentRole:
        return sourceModel_.data(sourceIndex, CvListModel::LanguageAccentRole);
    case LastModifiedLabelRole:
        return sourceModel_.data(sourceIndex, CvListModel::LastModifiedLabelRole);
    case LinkedApplicationCountLabelRole:
        return sourceModel_.data(sourceIndex, CvListModel::LinkedApplicationCountLabelRole);
    default:
        return {};
    }
}

QHash<int, QByteArray> DashboardRecentCvsModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {FileNameRole, "fileName"},
        {CategoryRole, "category"},
        {CategoryAccentRole, "categoryAccent"},
        {LanguageRole, "language"},
        {LanguageAccentRole, "languageAccent"},
        {LastModifiedLabelRole, "lastModifiedLabel"},
        {LinkedApplicationCountLabelRole, "linkedApplicationCountLabel"},
    };
}

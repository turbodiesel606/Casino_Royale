#include "DashboardRecentApplicationsModel.h"

#include <algorithm>

namespace {

constexpr int recentApplicationLimit = 5;

}

DashboardRecentApplicationsModel::DashboardRecentApplicationsModel(const JobApplicationListModel& sourceModel, QObject* parent)
    : QAbstractListModel(parent)
    , sourceModel_(sourceModel)
{
    connect(&sourceModel_, &QAbstractItemModel::rowsInserted, this, [this]() {
        beginResetModel();
        endResetModel();
    });
    connect(&sourceModel_, &QAbstractItemModel::modelReset, this, [this]() {
        beginResetModel();
        endResetModel();
    });
}

int DashboardRecentApplicationsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : std::min(recentApplicationLimit, sourceModel_.rowCount());
}

QVariant DashboardRecentApplicationsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const auto sourceIndex = sourceModel_.index(index.row(), 0);
    switch (role) {
    case IdRole:
        return sourceModel_.data(sourceIndex, JobApplicationListModel::IdRole);
    case JobTitleRole:
        return sourceModel_.data(sourceIndex, JobApplicationListModel::JobTitleRole);
    case CompanyNameRole:
        return sourceModel_.data(sourceIndex, JobApplicationListModel::CompanyNameRole);
    case StatusLabelRole:
        return sourceModel_.data(sourceIndex, JobApplicationListModel::StatusLabelRole);
    case StatusAccentRole:
        return sourceModel_.data(sourceIndex, JobApplicationListModel::StatusAccentRole);
    case AppliedDateLabelRole:
        return sourceModel_.data(sourceIndex, JobApplicationListModel::DateLabelRole);
    default:
        return {};
    }
}

QHash<int, QByteArray> DashboardRecentApplicationsModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {JobTitleRole, "jobTitle"},
        {CompanyNameRole, "companyName"},
        {StatusLabelRole, "statusLabel"},
        {StatusAccentRole, "statusAccent"},
        {AppliedDateLabelRole, "appliedDateLabel"},
    };
}

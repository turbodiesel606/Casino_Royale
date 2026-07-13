#include "LinkedApplicationListModel.hpp"

LinkedApplicationListModel::LinkedApplicationListModel(const JobApplicationListModel& applicationsModel, QObject* parent)
    : QAbstractListModel(parent)
    , applicationsModel_(applicationsModel)
{
    connect(&applicationsModel_, &QAbstractItemModel::rowsInserted, this, &LinkedApplicationListModel::refresh);
    connect(&applicationsModel_, &QAbstractItemModel::modelReset, this, &LinkedApplicationListModel::refresh);
    connect(&applicationsModel_, &QAbstractItemModel::dataChanged, this, &LinkedApplicationListModel::refresh);
}

int LinkedApplicationListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return sourceRows_.size();
}

QVariant LinkedApplicationListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= sourceRows_.size()) {
        return {};
    }

    const auto sourceRow = sourceRows_.at(index.row());
    switch (role) {
    case IdRole:
        return sourceData(sourceRow, JobApplicationListModel::IdRole);
    case JobTitleRole:
        return sourceData(sourceRow, JobApplicationListModel::JobTitleRole);
    case CompanyNameRole:
        return sourceData(sourceRow, JobApplicationListModel::CompanyNameRole);
    case StatusLabelRole:
        return sourceData(sourceRow, JobApplicationListModel::StatusLabelRole);
    case StatusAccentRole:
        return sourceData(sourceRow, JobApplicationListModel::StatusAccentRole);
    case DateLabelRole:
        return sourceData(sourceRow, JobApplicationListModel::DateLabelRole);
    case CvFileNameRole:
        return sourceData(sourceRow, JobApplicationListModel::CvFileNameRole);
    default:
        return {};
    }
}

QHash<int, QByteArray> LinkedApplicationListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {JobTitleRole, "jobTitle"},
        {CompanyNameRole, "companyName"},
        {StatusLabelRole, "statusLabel"},
        {StatusAccentRole, "statusAccent"},
        {DateLabelRole, "dateLabel"},
        {CvFileNameRole, "cvFileName"},
    };
}

void LinkedApplicationListModel::setCvId(const QString& cvId)
{
    if (cvId_ == cvId) {
        return;
    }

    beginResetModel();
    cvId_ = cvId;
    rebuildSourceRows();
    endResetModel();
}

QVariant LinkedApplicationListModel::sourceData(int sourceRow, int role) const
{
    return applicationsModel_.data(applicationsModel_.index(sourceRow, 0), role);
}

void LinkedApplicationListModel::rebuildSourceRows()
{
    sourceRows_.clear();

    for (int row = 0; row < applicationsModel_.rowCount(); ++row) {
        if (sourceData(row, JobApplicationListModel::CvIdRole).toString() == cvId_) {
            sourceRows_.append(row);
        }
    }
}

void LinkedApplicationListModel::refresh()
{
    beginResetModel();
    rebuildSourceRows();
    endResetModel();
}

#include "LinkedCompanyJobsModel.hpp"

LinkedCompanyJobsModel::LinkedCompanyJobsModel(const JobApplicationListModel& applicationsModel, QObject* parent)
    : QAbstractListModel(parent)
    , applicationsModel_(applicationsModel)
{
}

int LinkedCompanyJobsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return sourceRows_.size();
}

QVariant LinkedCompanyJobsModel::data(const QModelIndex& index, int role) const
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

QHash<int, QByteArray> LinkedCompanyJobsModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {JobTitleRole, "jobTitle"},
        {StatusLabelRole, "statusLabel"},
        {StatusAccentRole, "statusAccent"},
        {DateLabelRole, "dateLabel"},
        {CvFileNameRole, "cvFileName"},
    };
}

void LinkedCompanyJobsModel::setCompanyId(const QString& companyId)
{
    if (companyId_ == companyId) {
        return;
    }

    beginResetModel();
    companyId_ = companyId;
    rebuildSourceRows();
    endResetModel();
}

QVariant LinkedCompanyJobsModel::sourceData(int sourceRow, int role) const
{
    return applicationsModel_.data(applicationsModel_.index(sourceRow, 0), role);
}

void LinkedCompanyJobsModel::rebuildSourceRows()
{
    sourceRows_.clear();

    for (int row = 0; row < applicationsModel_.rowCount(); ++row) {
        if (sourceData(row, JobApplicationListModel::CompanyIdRole).toString() == companyId_) {
            sourceRows_.append(row);
        }
    }
}

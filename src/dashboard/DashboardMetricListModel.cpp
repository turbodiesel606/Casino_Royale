#include "DashboardMetricListModel.hpp"

#include <utility>

DashboardMetricListModel::DashboardMetricListModel(QVector<DashboardMetric> metrics, QObject* parent)
    : QAbstractListModel(parent)
    , metrics_(std::move(metrics))
{
}

int DashboardMetricListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : metrics_.size();
}

QVariant DashboardMetricListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= metrics_.size()) {
        return {};
    }

    const auto& metric = metrics_.at(index.row());
    switch (role) {
    case IconRole:
        return metric.icon_;
    case TitleRole:
        return metric.title_;
    case ValueRole:
        return metric.value_;
    case NoteRole:
        return metric.note_;
    case RatioRole:
        return metric.ratio_;
    case AccentRole:
        return metric.accent_;
    default:
        return {};
    }
}

QHash<int, QByteArray> DashboardMetricListModel::roleNames() const
{
    return {
        {IconRole, "icon"},
        {TitleRole, "title"},
        {ValueRole, "value"},
        {NoteRole, "note"},
        {RatioRole, "ratio"},
        {AccentRole, "accent"},
    };
}

void DashboardMetricListModel::setMetrics(QVector<DashboardMetric> metrics)
{
    beginResetModel();
    metrics_ = std::move(metrics);
    endResetModel();
}

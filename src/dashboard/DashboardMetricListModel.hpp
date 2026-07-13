#ifndef JOBTRACKER_SRC_DASHBOARD_DASHBOARDMETRICLISTMODEL_HPP
#define JOBTRACKER_SRC_DASHBOARD_DASHBOARDMETRICLISTMODEL_HPP

#include "DashboardMetric.hpp"

#include <QAbstractListModel>
#include <QVector>

class DashboardMetricListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IconRole = Qt::UserRole + 1,
        TitleRole,
        ValueRole,
        NoteRole,
        RatioRole,
        AccentRole
    };

    explicit DashboardMetricListModel(QVector<DashboardMetric> metrics, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setMetrics(QVector<DashboardMetric> metrics);

private:
    QVector<DashboardMetric> metrics_;
};

#endif // JOBTRACKER_SRC_DASHBOARD_DASHBOARDMETRICLISTMODEL_HPP

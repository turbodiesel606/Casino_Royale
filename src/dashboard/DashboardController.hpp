#ifndef JOBTRACKER_SRC_DASHBOARD_DASHBOARDCONTROLLER_HPP
#define JOBTRACKER_SRC_DASHBOARD_DASHBOARDCONTROLLER_HPP

#include "common/LimitedSortedProxyModel.hpp"
#include "DashboardMetricListModel.hpp"
#include "cvs/CvListModel.hpp"
#include "jobs/JobApplicationListModel.hpp"

#include <QObject>

class QAbstractItemModel;

class DashboardController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* statsModel READ statsModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* funnelModel READ funnelModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* recentApplicationsModel READ recentApplicationsModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* recentCvsModel READ recentCvsModel CONSTANT)

public:
    DashboardController(const JobApplicationListModel& applicationsModel, const CvListModel& cvModel, QObject* parent = nullptr);

    QAbstractItemModel* statsModel();
    QAbstractItemModel* funnelModel();
    QAbstractItemModel* recentApplicationsModel();
    QAbstractItemModel* recentCvsModel();

private:
    void refreshMetrics();

    const JobApplicationListModel& applicationsModel_;
    DashboardMetricListModel statsModel_;
    DashboardMetricListModel funnelModel_;
    LimitedSortedProxyModel recentApplicationsModel_;
    LimitedSortedProxyModel recentCvsModel_;
};

#endif // JOBTRACKER_SRC_DASHBOARD_DASHBOARDCONTROLLER_HPP

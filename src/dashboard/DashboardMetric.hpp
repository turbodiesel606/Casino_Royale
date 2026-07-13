#ifndef JOBTRACKER_SRC_DASHBOARD_DASHBOARDMETRIC_HPP
#define JOBTRACKER_SRC_DASHBOARD_DASHBOARDMETRIC_HPP

#include <QString>

struct DashboardMetric
{
    QString icon_;
    QString title_;
    QString value_;
    QString note_;
    double ratio_ = 0.0;
    QString accent_;
};

#endif // JOBTRACKER_SRC_DASHBOARD_DASHBOARDMETRIC_HPP

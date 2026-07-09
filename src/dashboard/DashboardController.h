#pragma once

#include "DashboardMetricListModel.h"
#include "DashboardRecentApplicationsModel.h"
#include "DashboardRecentCvsModel.h"
#include "cvs/CvListModel.h"
#include "jobs/JobApplicationListModel.h"

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
    DashboardRecentApplicationsModel recentApplicationsModel_;
    DashboardRecentCvsModel recentCvsModel_;
};

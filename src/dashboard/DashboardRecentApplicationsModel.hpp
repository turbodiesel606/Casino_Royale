#ifndef JOBTRACKER_SRC_DASHBOARD_DASHBOARDRECENTAPPLICATIONSMODEL_HPP
#define JOBTRACKER_SRC_DASHBOARD_DASHBOARDRECENTAPPLICATIONSMODEL_HPP

#include "jobs/JobApplicationListModel.hpp"

#include <QAbstractListModel>

class DashboardRecentApplicationsModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        JobTitleRole,
        CompanyNameRole,
        StatusLabelRole,
        StatusAccentRole,
        AppliedDateLabelRole
    };

    explicit DashboardRecentApplicationsModel(const JobApplicationListModel& sourceModel, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    const JobApplicationListModel& sourceModel_;
};

#endif // JOBTRACKER_SRC_DASHBOARD_DASHBOARDRECENTAPPLICATIONSMODEL_HPP

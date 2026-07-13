#ifndef JOBTRACKER_SRC_DASHBOARD_DASHBOARDRECENTCVSMODEL_HPP
#define JOBTRACKER_SRC_DASHBOARD_DASHBOARDRECENTCVSMODEL_HPP

#include "cvs/CvListModel.hpp"

#include <QAbstractListModel>

class DashboardRecentCvsModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        FileNameRole,
        CategoryRole,
        CategoryAccentRole,
        LanguageRole,
        LanguageAccentRole,
        LastModifiedLabelRole,
        LinkedApplicationCountLabelRole
    };

    explicit DashboardRecentCvsModel(const CvListModel& sourceModel, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    const CvListModel& sourceModel_;
};

#endif // JOBTRACKER_SRC_DASHBOARD_DASHBOARDRECENTCVSMODEL_HPP

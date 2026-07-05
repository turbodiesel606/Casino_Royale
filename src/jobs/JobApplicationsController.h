#pragma once

#include "common/RoleFilterProxyModel.h"
#include "JobApplicationListModel.h"

#include <QObject>
#include <QStringList>
#include <QVariantMap>

class QAbstractItemModel;

class JobApplicationsController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* applicationsModel READ applicationsModel CONSTANT)
    Q_PROPERTY(int applicationCount READ applicationCount NOTIFY applicationsModelChanged)
    Q_PROPERTY(int selectedApplicationIndex READ selectedApplicationIndex NOTIFY selectedApplicationChanged)
    Q_PROPERTY(QString selectedApplicationId READ selectedApplicationId NOTIFY selectedApplicationChanged)
    Q_PROPERTY(QVariantMap selectedApplication READ selectedApplication NOTIFY selectedApplicationChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filtersChanged)
    Q_PROPERTY(QString statusFilter READ statusFilter WRITE setStatusFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString resultSummary READ resultSummary NOTIFY resultSummaryChanged)

public:
    explicit JobApplicationsController(QObject* parent = nullptr);

    QAbstractItemModel* applicationsModel();
    JobApplicationListModel& jobApplicationListModel();
    const JobApplicationListModel& jobApplicationListModel() const;
    int applicationCount() const;
    int selectedApplicationIndex() const;
    QString selectedApplicationId() const;
    QVariantMap selectedApplication() const;
    QString searchText() const;
    QString statusFilter() const;
    QString resultSummary() const;

    Q_INVOKABLE void selectApplication(int index);
    Q_INVOKABLE void setSearchText(const QString& text);
    Q_INVOKABLE void setStatusFilter(const QString& status);
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE QStringList validateSelectedApplication() const;

signals:
    void applicationsModelChanged();
    void selectedApplicationChanged();
    void filtersChanged();
    void resultSummaryChanged();

private:
    const JobApplication* selectedSourceApplication() const;
    int selectedSourceRow() const;
    void refreshSelectionAfterFilterChange();
    QVariantMap applicationToMap(const JobApplication& application) const;

    JobApplicationListModel applicationsModel_;
    RoleFilterProxyModel filteredApplicationsModel_;
    QString searchText_;
    QString statusFilter_;
    int selectedApplicationIndex_ = 0;
};

#ifndef JOBTRACKER_SRC_DIRECTORY_LINKEDCOMPANYJOBSMODEL_HPP
#define JOBTRACKER_SRC_DIRECTORY_LINKEDCOMPANYJOBSMODEL_HPP

#include "jobs/JobApplicationListModel.hpp"

#include <QAbstractListModel>
#include <QVector>

class LinkedCompanyJobsModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        JobTitleRole,
        StatusLabelRole,
        StatusAccentRole,
        DateLabelRole,
        CvFileNameRole
    };

    explicit LinkedCompanyJobsModel(const JobApplicationListModel& applicationsModel, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setCompanyId(const QString& companyId);

private:
    QVariant sourceData(int sourceRow, int role) const;
    void refreshSourceRows();
    void rebuildSourceRows();

    const JobApplicationListModel& applicationsModel_;
    QString companyId_;
    QVector<int> sourceRows_;
};

#endif // JOBTRACKER_SRC_DIRECTORY_LINKEDCOMPANYJOBSMODEL_HPP

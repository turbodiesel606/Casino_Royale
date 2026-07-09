#pragma once

#include "jobs/JobApplicationListModel.h"

#include <QAbstractListModel>
#include <QVector>

class LinkedApplicationListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        JobTitleRole,
        CompanyNameRole,
        StatusLabelRole,
        StatusAccentRole,
        DateLabelRole,
        CvFileNameRole
    };

    explicit LinkedApplicationListModel(const JobApplicationListModel& applicationsModel, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setCvId(const QString& cvId);

private:
    QVariant sourceData(int sourceRow, int role) const;
    void rebuildSourceRows();
    void refresh();

    const JobApplicationListModel& applicationsModel_;
    QString cvId_;
    QVector<int> sourceRows_;
};

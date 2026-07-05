#pragma once

#include "JobApplication.h"

#include <QAbstractListModel>
#include <QVector>

class JobApplicationListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        CompanyIdRole,
        CompanyNameRole,
        CompanyInitialsRole,
        CompanyAccentRole,
        JobTitleRole,
        JobUrlRole,
        WorkFormatRole,
        CityRole,
        SalaryRole,
        StatusRole,
        StatusLabelRole,
        StatusAccentRole,
        AppliedDateRole,
        DateLabelRole,
        NextStepRole,
        CvIdRole,
        CvFileNameRole,
        DescriptionRole,
        RequirementsRole,
        TechStackRole,
        NotesRole
    };

    explicit JobApplicationListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const JobApplication* applicationAt(int row) const;

private:
    QVector<JobApplication> applications_;
};

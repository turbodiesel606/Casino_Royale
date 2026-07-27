#ifndef JOBTRACKER_SRC_JOBS_JOBAPPLICATIONLISTMODEL_HPP
#define JOBTRACKER_SRC_JOBS_JOBAPPLICATIONLISTMODEL_HPP

#include "JobApplication.hpp"

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
        NotesRole,
        StatusValueRole,
        WorkFormatValueRole,
        AppliedDateValueRole,
        CreatedAtRole,
        UpdatedAtRole
    };

    explicit JobApplicationListModel(QObject* parent = nullptr);
    explicit JobApplicationListModel(QVector<JobApplication> applications, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const JobApplication* applicationAt(int row) const;
    void setApplications(QVector<JobApplication> applications);
    void appendApplication(JobApplication application);

private:
    QVector<JobApplication> applications_;
};

#endif // JOBTRACKER_SRC_JOBS_JOBAPPLICATIONLISTMODEL_HPP

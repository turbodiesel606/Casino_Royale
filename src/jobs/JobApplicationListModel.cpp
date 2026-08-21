#include "JobApplicationListModel.hpp"

#include <QLocale>

#include <utility>

namespace {

QString companyInitials(const QString& companyName)
{
    return companyName.left(2).toUpper();
}

QString statusAccent(JobStatus status)
{
    if (status == JobStatus::Interview) {
        return QStringLiteral("#ffbd21");
    }
    if (status == JobStatus::Offer) {
        return QStringLiteral("#38c86b");
    }
    if (status == JobStatus::Rejected) {
        return QStringLiteral("#ff4b49");
    }
    if (status == JobStatus::TestTask) {
        return QStringLiteral("#16c5dd");
    }
    return QStringLiteral("#c2c7cb");
}

QString dateLabel(const QDate& date)
{
    return date.isValid()
        ? QLocale::c().toString(date, QStringLiteral("MMM d, yyyy"))
        : QString{};
}

QVariant roleValue(const JobApplication& application, int role)
{
    switch (role) {
    case JobApplicationListModel::IdRole:
        return application.id_;
    case JobApplicationListModel::CompanyIdRole:
        return application.companyId_;
    case JobApplicationListModel::CompanyNameRole:
        return application.companyName_;
    case JobApplicationListModel::CompanyInitialsRole:
        return companyInitials(application.companyName_);
    case JobApplicationListModel::CompanyAccentRole:
        return QStringLiteral("#146ce0");
    case JobApplicationListModel::JobTitleRole:
        return application.jobTitle_;
    case JobApplicationListModel::JobUrlRole:
        return application.jobUrl_.toString();
    case JobApplicationListModel::WorkFormatRole:
        return workFormatToString(application.workFormat_);
    case JobApplicationListModel::CityRole:
        return application.city_;
    case JobApplicationListModel::SalaryRole:
        return application.salary_;
    case JobApplicationListModel::StatusRole:
    case JobApplicationListModel::StatusLabelRole:
        return jobStatusToString(application.status_);
    case JobApplicationListModel::StatusAccentRole:
        return statusAccent(application.status_);
    case JobApplicationListModel::AppliedDateRole:
        return application.appliedDate_.toString(Qt::ISODate);
    case JobApplicationListModel::DateLabelRole:
        return dateLabel(application.appliedDate_);
    case JobApplicationListModel::NextStepRole:
        return application.nextStep_;
    case JobApplicationListModel::CvIdRole:
        return application.cvId_;
    case JobApplicationListModel::CvFileNameRole:
        return application.cvFileName_;
    case JobApplicationListModel::DescriptionRole:
        return application.description_;
    case JobApplicationListModel::RequirementsRole:
        return application.requirements_;
    case JobApplicationListModel::TechStackRole:
        return application.techStack_;
    case JobApplicationListModel::NotesRole:
        return application.notes_;
    case JobApplicationListModel::StatusValueRole:
        return static_cast<int>(application.status_);
    case JobApplicationListModel::WorkFormatValueRole:
        return static_cast<int>(application.workFormat_);
    case JobApplicationListModel::AppliedDateValueRole:
        return application.appliedDate_;
    case JobApplicationListModel::CreatedAtRole:
        return application.createdAt_;
    case JobApplicationListModel::UpdatedAtRole:
        return application.updatedAt_;
    default:
        return {};
    }
}

QList<int> changedRoles(
    const JobApplication& previous,
    const JobApplication& current)
{
    QList<int> roles;
    const auto add = [&roles](std::initializer_list<int> values) {
        for (const auto role : values) {
            if (!roles.contains(role)) {
                roles.append(role);
            }
        }
    };

    if (previous.companyId_ != current.companyId_) {
        add({JobApplicationListModel::CompanyIdRole});
    }
    if (previous.companyName_ != current.companyName_) {
        add({JobApplicationListModel::CompanyNameRole,
             JobApplicationListModel::CompanyInitialsRole});
    }
    if (previous.jobTitle_ != current.jobTitle_) {
        add({JobApplicationListModel::JobTitleRole});
    }
    if (previous.jobUrl_ != current.jobUrl_) {
        add({JobApplicationListModel::JobUrlRole});
    }
    if (previous.workFormat_ != current.workFormat_) {
        add({JobApplicationListModel::WorkFormatRole,
             JobApplicationListModel::WorkFormatValueRole});
    }
    if (previous.city_ != current.city_) {
        add({JobApplicationListModel::CityRole});
    }
    if (previous.salary_ != current.salary_) {
        add({JobApplicationListModel::SalaryRole});
    }
    if (previous.status_ != current.status_) {
        add({JobApplicationListModel::StatusRole,
             JobApplicationListModel::StatusLabelRole,
             JobApplicationListModel::StatusAccentRole,
             JobApplicationListModel::StatusValueRole});
    }
    if (previous.appliedDate_ != current.appliedDate_) {
        add({JobApplicationListModel::AppliedDateRole,
             JobApplicationListModel::DateLabelRole,
             JobApplicationListModel::AppliedDateValueRole});
    }
    if (previous.nextStep_ != current.nextStep_) {
        add({JobApplicationListModel::NextStepRole});
    }
    if (previous.cvId_ != current.cvId_) {
        add({JobApplicationListModel::CvIdRole});
    }
    if (previous.cvFileName_ != current.cvFileName_) {
        add({JobApplicationListModel::CvFileNameRole});
    }
    if (previous.description_ != current.description_) {
        add({JobApplicationListModel::DescriptionRole});
    }
    if (previous.requirements_ != current.requirements_) {
        add({JobApplicationListModel::RequirementsRole});
    }
    if (previous.techStack_ != current.techStack_) {
        add({JobApplicationListModel::TechStackRole});
    }
    if (previous.notes_ != current.notes_) {
        add({JobApplicationListModel::NotesRole});
    }
    if (previous.createdAt_ != current.createdAt_) {
        add({JobApplicationListModel::CreatedAtRole});
    }
    if (previous.updatedAt_ != current.updatedAt_) {
        add({JobApplicationListModel::UpdatedAtRole});
    }
    return roles;
}

}

JobApplicationListModel::JobApplicationListModel(QObject* parent)
    : JobApplicationListModel(QVector<JobApplication>{}, parent)
{
}

JobApplicationListModel::JobApplicationListModel(QVector<JobApplication> applications, QObject* parent)
    : QAbstractListModel(parent)
    , applications_(std::move(applications))
{
}

int JobApplicationListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return applications_.size();
}

QVariant JobApplicationListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= applications_.size()) {
        return {};
    }

    return roleValue(applications_.at(index.row()), role);
}

QHash<int, QByteArray> JobApplicationListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {CompanyIdRole, "companyId"},
        {CompanyNameRole, "companyName"},
        {CompanyInitialsRole, "companyInitials"},
        {CompanyAccentRole, "companyAccent"},
        {JobTitleRole, "jobTitle"},
        {JobUrlRole, "jobUrl"},
        {WorkFormatRole, "workFormat"},
        {CityRole, "city"},
        {SalaryRole, "salary"},
        {StatusRole, "status"},
        {StatusLabelRole, "statusLabel"},
        {StatusAccentRole, "statusAccent"},
        {AppliedDateRole, "appliedDate"},
        {DateLabelRole, "dateLabel"},
        {NextStepRole, "nextStep"},
        {CvIdRole, "cvId"},
        {CvFileNameRole, "cvFileName"},
        {DescriptionRole, "description"},
        {RequirementsRole, "requirements"},
        {TechStackRole, "techStack"},
        {NotesRole, "notes"},
        {StatusValueRole, "statusValue"},
        {WorkFormatValueRole, "workFormatValue"},
        {AppliedDateValueRole, "appliedDateValue"},
        {CreatedAtRole, "createdAt"},
        {UpdatedAtRole, "updatedAt"},
    };
}

const JobApplication* JobApplicationListModel::applicationAt(int row) const
{
    if (row < 0 || row >= applications_.size()) {
        return nullptr;
    }
    return &applications_.at(row);
}

void JobApplicationListModel::setApplications(QVector<JobApplication> applications)
{
    beginResetModel();
    applications_ = std::move(applications);
    endResetModel();
}

void JobApplicationListModel::appendApplication(JobApplication application)
{
    const auto row = applications_.size();
    beginInsertRows({}, row, row);
    applications_.append(std::move(application));
    endInsertRows();
}

bool JobApplicationListModel::updateApplication(JobApplication application)
{
    for (int row = 0; row < applications_.size(); ++row) {
        if (applications_.at(row).id_ != application.id_) {
            continue;
        }

        const auto roles = changedRoles(applications_.at(row), application);
        applications_[row] = std::move(application);
        if (!roles.isEmpty()) {
            const auto modelIndex = index(row, 0);
            emit dataChanged(modelIndex, modelIndex, roles);
        }
        return true;
    }
    return false;
}

int JobApplicationListModel::removeApplications(const QStringList& applicationIds)
{
    int removed = 0;
    for (int row = applications_.size() - 1; row >= 0; --row) {
        if (!applicationIds.contains(applications_.at(row).id_)) {
            continue;
        }
        beginRemoveRows({}, row, row);
        applications_.removeAt(row);
        endRemoveRows();
        ++removed;
    }
    return removed;
}

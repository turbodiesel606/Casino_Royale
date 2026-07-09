#include "JobApplicationListModel.h"

#include <utility>

namespace {

QString statusAccent(const QString& status)
{
    if (status == QStringLiteral("Interview")) {
        return QStringLiteral("#ffbd21");
    }
    if (status == QStringLiteral("Offer")) {
        return QStringLiteral("#38c86b");
    }
    if (status == QStringLiteral("Rejected")) {
        return QStringLiteral("#ff4b49");
    }
    if (status == QStringLiteral("Test Task")) {
        return QStringLiteral("#16c5dd");
    }
    return QStringLiteral("#c2c7cb");
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
        return application.companyInitials_;
    case JobApplicationListModel::CompanyAccentRole:
        return application.companyAccent_;
    case JobApplicationListModel::JobTitleRole:
        return application.jobTitle_;
    case JobApplicationListModel::JobUrlRole:
        return application.jobUrl_;
    case JobApplicationListModel::WorkFormatRole:
        return application.workFormat_;
    case JobApplicationListModel::CityRole:
        return application.city_;
    case JobApplicationListModel::SalaryRole:
        return application.salary_;
    case JobApplicationListModel::StatusRole:
    case JobApplicationListModel::StatusLabelRole:
        return application.status_;
    case JobApplicationListModel::StatusAccentRole:
        return statusAccent(application.status_);
    case JobApplicationListModel::AppliedDateRole:
        return application.appliedDate_;
    case JobApplicationListModel::DateLabelRole:
        return application.dateLabel_;
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
    default:
        return {};
    }
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
    };
}

const JobApplication* JobApplicationListModel::applicationAt(int row) const
{
    if (row < 0 || row >= applications_.size()) {
        return nullptr;
    }
    return &applications_.at(row);
}

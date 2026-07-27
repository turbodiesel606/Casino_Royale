#ifndef JOBTRACKER_SRC_JOBS_JOBAPPLICATION_HPP
#define JOBTRACKER_SRC_JOBS_JOBAPPLICATION_HPP

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QUrl>

// Closed domain choices stored by their existing schema-v3 text values.
enum class JobStatus
{
    Unknown,
    Applied,
    Interview,
    Offer,
    TestTask,
    Rejected
};

enum class WorkFormat
{
    Unknown,
    Unspecified,
    Remote,
    Hybrid,
    OnSite
};

JobStatus jobStatusFromString(const QString& value);
QString jobStatusToString(JobStatus status);
WorkFormat workFormatFromString(const QString& value);
QString workFormatToString(WorkFormat workFormat);

struct JobApplication
{
    QString id_;
    QString companyId_;
    QString companyName_;
    QString jobTitle_;
    QUrl jobUrl_;
    WorkFormat workFormat_ = WorkFormat::Unspecified;
    QString city_;
    QString salary_;
    JobStatus status_ = JobStatus::Unknown;
    QDate appliedDate_;
    QString nextStep_;
    QString cvId_;
    QString cvFileName_;
    QString description_;
    QString requirements_;
    QStringList techStack_;
    QString notes_;
    QDateTime createdAt_;
    QDateTime updatedAt_;
};

#endif // JOBTRACKER_SRC_JOBS_JOBAPPLICATION_HPP

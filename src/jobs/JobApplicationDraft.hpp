#ifndef JOBTRACKER_SRC_JOBS_JOBAPPLICATIONDRAFT_HPP
#define JOBTRACKER_SRC_JOBS_JOBAPPLICATIONDRAFT_HPP

#include "JobApplication.hpp"

#include <QString>
#include <QStringList>

struct JobApplicationDraft
{
    QString companyName_;
    QString jobTitle_;
    QString jobUrl_;
    QString workFormat_;
    QString city_;
    QString salary_;
    QString status_;
    QString appliedDate_;
    QString nextStep_;
    QString description_;
    QString requirements_;
    QStringList techStack_;
    QString notes_;
};

// Typed, normalized input used after the QML string boundary has been mapped.
struct NormalizedJobApplicationDraft
{
    QString companyName_;
    QString jobTitle_;
    QUrl jobUrl_;
    WorkFormat workFormat_ = WorkFormat::Unspecified;
    QString city_;
    QString salary_;
    JobStatus status_ = JobStatus::Applied;
    QDate appliedDate_;
    QString nextStep_;
    QString description_;
    QString requirements_;
    QStringList techStack_;
    QString notes_;
};

#endif // JOBTRACKER_SRC_JOBS_JOBAPPLICATIONDRAFT_HPP

#ifndef JOBTRACKER_SRC_JOBS_JOBAPPLICATION_HPP
#define JOBTRACKER_SRC_JOBS_JOBAPPLICATION_HPP

#include <QString>
#include <QStringList>

struct JobApplication
{
    QString id_;
    QString companyId_;
    QString companyName_;
    QString companyInitials_;
    QString companyAccent_;
    QString jobTitle_;
    QString jobUrl_;
    QString workFormat_;
    QString city_;
    QString salary_;
    QString status_;
    QString appliedDate_;
    QString dateLabel_;
    QString nextStep_;
    QString cvId_;
    QString cvFileName_;
    QString description_;
    QString requirements_;
    QStringList techStack_;
    QString notes_;
};

#endif // JOBTRACKER_SRC_JOBS_JOBAPPLICATION_HPP

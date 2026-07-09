#pragma once

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

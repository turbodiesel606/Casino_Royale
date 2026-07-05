#pragma once

#include <QString>
#include <QVector>

struct ContactInteraction
{
    QString type_;
    QString title_;
    QString timestampLabel_;
    QString notes_;
};

struct Contact
{
    QString id_;
    QString displayName_;
    QString initials_;
    QString avatarAccent_;
    QString roleTitle_;
    QString companyId_;
    QString companyName_;
    QString relatedApplicationId_;
    QString relatedApplicationTitle_;
    QString email_;
    QString telegram_;
    QString linkedin_;
    QString lastContactLabel_;
    QString notes_;
    QVector<ContactInteraction> interactions_;
};

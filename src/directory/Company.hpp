#ifndef JOBTRACKER_SRC_DIRECTORY_COMPANY_HPP
#define JOBTRACKER_SRC_DIRECTORY_COMPANY_HPP

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QUrl>

struct Company
{
    QString id_;
    QString name_;
    QUrl website_;
    int openJobCount_ = 0;
    int contactCount_ = 0;
    QDateTime lastActivityAt_;
    QString description_;
    QString notes_;
    QDateTime createdAt_;
    QDateTime updatedAt_;
};

Q_DECLARE_METATYPE(Company)

#endif // JOBTRACKER_SRC_DIRECTORY_COMPANY_HPP

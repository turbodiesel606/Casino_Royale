#pragma once

#include <QString>

struct Company
{
    QString id_;
    QString name_;
    QString website_;
    QString logoText_;
    QString logoAccent_;
    int openJobCount_ = 0;
    int contactCount_ = 0;
    QString lastActivityLabel_;
    QString description_;
    QString notes_;
};

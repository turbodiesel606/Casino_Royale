#pragma once

#include <QString>

struct DashboardMetric
{
    QString icon_;
    QString title_;
    QString value_;
    QString note_;
    double ratio_ = 0.0;
    QString accent_;
};

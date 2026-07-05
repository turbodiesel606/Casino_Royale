#pragma once

#include <QStringList>

struct ValidationResult
{
    bool isValid_ = true;
    QStringList messages_;
};

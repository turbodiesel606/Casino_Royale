#pragma once

#include "ValidationResult.h"

#include <QVariantMap>

class ValidationService final
{
public:
    static ValidationResult validateRequiredFields(const QVariantMap& values, const QStringList& fieldNames);
    static ValidationResult validateHttpUrl(const QString& fieldName, const QString& url, bool required);
};

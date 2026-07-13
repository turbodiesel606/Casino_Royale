#ifndef JOBTRACKER_SRC_COMMON_VALIDATIONSERVICE_HPP
#define JOBTRACKER_SRC_COMMON_VALIDATIONSERVICE_HPP

#include "ValidationResult.hpp"

#include <QVariantMap>

class ValidationService final
{
public:
    static ValidationResult validateRequiredFields(const QVariantMap& values, const QStringList& fieldNames);
    static ValidationResult validateHttpUrl(const QString& fieldName, const QString& url, bool required);
};

#endif // JOBTRACKER_SRC_COMMON_VALIDATIONSERVICE_HPP

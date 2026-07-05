#include "ValidationService.h"

#include <QUrl>

ValidationResult ValidationService::validateRequiredFields(const QVariantMap& values, const QStringList& fieldNames)
{
    ValidationResult result;

    for (const auto& fieldName : fieldNames) {
        if (values.value(fieldName).toString().trimmed().isEmpty()) {
            result.isValid_ = false;
            result.messages_.append(QStringLiteral("%1 is required.").arg(fieldName));
        }
    }

    return result;
}

ValidationResult ValidationService::validateHttpUrl(const QString& fieldName, const QString& url, bool required)
{
    ValidationResult result;
    const auto trimmedUrl = url.trimmed();

    if (trimmedUrl.isEmpty()) {
        if (required) {
            result.isValid_ = false;
            result.messages_.append(QStringLiteral("%1 is required.").arg(fieldName));
        }
        return result;
    }

    const QUrl parsedUrl(trimmedUrl);
    if (!parsedUrl.isValid() || parsedUrl.scheme().isEmpty() || parsedUrl.host().isEmpty()
        || (parsedUrl.scheme() != QStringLiteral("http") && parsedUrl.scheme() != QStringLiteral("https"))) {
        result.isValid_ = false;
        result.messages_.append(QStringLiteral("%1 must be a valid HTTP or HTTPS URL.").arg(fieldName));
    }

    return result;
}

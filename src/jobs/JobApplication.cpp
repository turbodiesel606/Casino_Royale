#include "JobApplication.hpp"

JobStatus jobStatusFromString(const QString& value)
{
    const auto normalized = value.trimmed();
    if (normalized.compare(QStringLiteral("Applied"), Qt::CaseInsensitive) == 0) {
        return JobStatus::Applied;
    }
    if (normalized.compare(QStringLiteral("Interview"), Qt::CaseInsensitive) == 0) {
        return JobStatus::Interview;
    }
    if (normalized.compare(QStringLiteral("Offer"), Qt::CaseInsensitive) == 0) {
        return JobStatus::Offer;
    }
    if (normalized.compare(QStringLiteral("Test Task"), Qt::CaseInsensitive) == 0) {
        return JobStatus::TestTask;
    }
    if (normalized.compare(QStringLiteral("Rejected"), Qt::CaseInsensitive) == 0) {
        return JobStatus::Rejected;
    }
    return JobStatus::Unknown;
}

QString jobStatusToString(JobStatus status)
{
    switch (status) {
    case JobStatus::Applied:
        return QStringLiteral("Applied");
    case JobStatus::Interview:
        return QStringLiteral("Interview");
    case JobStatus::Offer:
        return QStringLiteral("Offer");
    case JobStatus::TestTask:
        return QStringLiteral("Test Task");
    case JobStatus::Rejected:
        return QStringLiteral("Rejected");
    case JobStatus::Unknown:
        return {};
    }
    return {};
}

WorkFormat workFormatFromString(const QString& value)
{
    const auto normalized = value.trimmed();
    if (normalized.isEmpty()) {
        return WorkFormat::Unspecified;
    }
    if (normalized.compare(QStringLiteral("Remote"), Qt::CaseInsensitive) == 0) {
        return WorkFormat::Remote;
    }
    if (normalized.compare(QStringLiteral("Hybrid"), Qt::CaseInsensitive) == 0) {
        return WorkFormat::Hybrid;
    }
    if (normalized.compare(QStringLiteral("On-site"), Qt::CaseInsensitive) == 0) {
        return WorkFormat::OnSite;
    }
    return WorkFormat::Unknown;
}

QString workFormatToString(WorkFormat workFormat)
{
    switch (workFormat) {
    case WorkFormat::Remote:
        return QStringLiteral("Remote");
    case WorkFormat::Hybrid:
        return QStringLiteral("Hybrid");
    case WorkFormat::OnSite:
        return QStringLiteral("On-site");
    case WorkFormat::Unknown:
    case WorkFormat::Unspecified:
        return {};
    }
    return {};
}

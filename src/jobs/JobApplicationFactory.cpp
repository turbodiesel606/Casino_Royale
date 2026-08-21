#include "JobApplicationFactory.hpp"

#include "cvs/CvDocument.hpp"
#include "directory/Company.hpp"

#include <QDateTime>
#include <QSet>
#include <QUuid>

namespace {

QStringList normalizedTechnologies(const QStringList& values)
{
    QStringList result;
    QSet<QString> seen;
    for (const auto& value : values) {
        const auto trimmed = value.trimmed();
        const auto key = trimmed.toCaseFolded();
        if (!trimmed.isEmpty() && !seen.contains(key)) {
            seen.insert(key);
            result.append(trimmed);
        }
    }
    return result;
}

QDateTime currentUtcSecond()
{
    return QDateTime::fromString(
               QDateTime::currentDateTimeUtc().toString(Qt::ISODate),
               Qt::ISODate)
        .toUTC();
}

}

NormalizedJobApplicationDraft JobApplicationFactory::normalize(const JobApplicationDraft& draft)
{
    NormalizedJobApplicationDraft normalized;
    normalized.companyName_ = draft.companyName_.trimmed();
    normalized.jobTitle_ = draft.jobTitle_.trimmed();
    normalized.jobUrl_ = QUrl{draft.jobUrl_.trimmed(), QUrl::StrictMode};
    normalized.workFormat_ = workFormatFromString(draft.workFormat_);
    normalized.city_ = draft.city_.trimmed();
    normalized.salary_ = draft.salary_.trimmed();
    normalized.status_ = draft.status_.trimmed().isEmpty()
        ? JobStatus::Applied
        : jobStatusFromString(draft.status_);
    normalized.appliedDate_ = draft.appliedDate_.trimmed().isEmpty()
        ? QDate::currentDate()
        : QDate::fromString(draft.appliedDate_.trimmed(), Qt::ISODate);
    normalized.nextStep_ = draft.nextStep_.trimmed();
    normalized.description_ = draft.description_.trimmed();
    normalized.requirements_ = draft.requirements_.trimmed();
    normalized.techStack_ = normalizedTechnologies(draft.techStack_);
    normalized.notes_ = draft.notes_.trimmed();
    return normalized;
}

JobApplication JobApplicationFactory::create(
    const NormalizedJobApplicationDraft& draft,
    const Company& company,
    const CvDocument& cvDocument)
{
    const auto now = currentUtcSecond();
    JobApplication application;
    application.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
    application.companyId_ = company.id_;
    application.companyName_ = company.name_;
    application.jobTitle_ = draft.jobTitle_;
    application.jobUrl_ = draft.jobUrl_;
    application.workFormat_ = draft.workFormat_;
    application.city_ = draft.city_;
    application.salary_ = draft.salary_;
    application.status_ = draft.status_;
    application.appliedDate_ = draft.appliedDate_;
    application.nextStep_ = draft.nextStep_;
    application.cvId_ = cvDocument.id_;
    application.cvFileName_ = cvDocument.originalFileName_;
    application.description_ = draft.description_;
    application.requirements_ = draft.requirements_;
    application.techStack_ = draft.techStack_;
    application.notes_ = draft.notes_;
    application.createdAt_ = now;
    application.updatedAt_ = now;
    return application;
}

JobApplication JobApplicationFactory::update(
    const JobApplication& existing,
    const NormalizedJobApplicationDraft& draft,
    const Company& company,
    const CvDocument* replacementCv)
{
    auto application = existing;
    application.companyId_ = company.id_;
    application.companyName_ = company.name_;
    application.jobTitle_ = draft.jobTitle_;
    application.jobUrl_ = draft.jobUrl_;
    application.workFormat_ = draft.workFormat_;
    application.city_ = draft.city_;
    application.salary_ = draft.salary_;
    application.status_ = draft.status_;
    application.appliedDate_ = draft.appliedDate_;
    application.nextStep_ = draft.nextStep_;
    if (replacementCv != nullptr) {
        application.cvId_ = replacementCv->id_;
        application.cvFileName_ = replacementCv->originalFileName_;
    }
    application.description_ = draft.description_;
    application.requirements_ = draft.requirements_;
    application.techStack_ = draft.techStack_;
    application.notes_ = draft.notes_;
    application.updatedAt_ = QDateTime::currentDateTimeUtc();
    if (application.updatedAt_ <= existing.updatedAt_) {
        application.updatedAt_ = existing.updatedAt_.addMSecs(1);
    }
    return application;
}

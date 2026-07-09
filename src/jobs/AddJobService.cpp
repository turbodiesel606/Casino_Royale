#include "AddJobService.h"

#include "JobRepository.h"
#include "cvs/CvImportService.h"

#include <QDate>
#include <QFile>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QUrl>
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

}

AddJobService::AddJobService(
    QSqlDatabase& database,
    JobRepository& jobRepository,
    CvImportService& cvImportService)
    : database_(database)
    , jobRepository_(jobRepository)
    , cvImportService_(cvImportService)
{
}

AddJobResult AddJobService::create(const JobApplicationDraft& draft, const QUrl& selectedCvUrl) const
{
    AddJobResult result;
    const auto jobTitle = draft.jobTitle_.trimmed();
    const auto companyName = draft.companyName_.trimmed();
    const auto status = draft.status_.trimmed().isEmpty() ? QStringLiteral("Applied") : draft.status_.trimmed();
    const auto appliedDate = draft.appliedDate_.trimmed().isEmpty()
        ? QDate::currentDate().toString(Qt::ISODate)
        : draft.appliedDate_.trimmed();

    if (jobTitle.isEmpty()) {
        result.fieldErrors_.insert(QStringLiteral("jobTitle"), QStringLiteral("Job title is required."));
    }
    if (companyName.isEmpty()) {
        result.fieldErrors_.insert(QStringLiteral("companyName"), QStringLiteral("Company is required."));
    }
    if (!QDate::fromString(appliedDate, Qt::ISODate).isValid()) {
        result.fieldErrors_.insert(QStringLiteral("appliedDate"), QStringLiteral("Use date format yyyy-MM-dd."));
    }
    if (selectedCvUrl.isEmpty()) {
        result.fieldErrors_.insert(QStringLiteral("cv"), QStringLiteral("Select a CV."));
    }

    const auto jobUrl = draft.jobUrl_.trimmed();
    if (!jobUrl.isEmpty()) {
        const QUrl parsedUrl(jobUrl);
        if (!parsedUrl.isValid()
            || (parsedUrl.scheme() != QStringLiteral("http") && parsedUrl.scheme() != QStringLiteral("https"))) {
            result.fieldErrors_.insert(QStringLiteral("jobUrl"), QStringLiteral("Use a valid HTTP or HTTPS URL."));
        }
    }
    if (!result.fieldErrors_.isEmpty()) {
        result.message_ = QStringLiteral("Please correct the highlighted fields.");
        return result;
    }

    if (!database_.transaction()) {
        result.message_ = database_.lastError().text();
        return result;
    }

    QString copiedFilePath;
    try {
        const auto cvImport = cvImportService_.importDocument(selectedCvUrl);
        copiedFilePath = cvImport.copiedFilePath_;

        JobApplication application;
        application.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
        application.companyName_ = companyName;
        application.companyInitials_ = companyName.left(2).toUpper();
        application.companyAccent_ = QStringLiteral("#146ce0");
        application.jobTitle_ = jobTitle;
        application.jobUrl_ = jobUrl;
        application.workFormat_ = draft.workFormat_.trimmed();
        application.city_ = draft.city_.trimmed();
        application.salary_ = draft.salary_.trimmed();
        application.status_ = status;
        application.appliedDate_ = appliedDate;
        application.dateLabel_ = QDate::fromString(appliedDate, Qt::ISODate).toString(QStringLiteral("MMM d, yyyy"));
        application.nextStep_ = draft.nextStep_.trimmed();
        application.cvId_ = cvImport.document_.id_;
        application.cvFileName_ = cvImport.document_.originalFileName_;
        application.description_ = draft.description_.trimmed();
        application.requirements_ = draft.requirements_.trimmed();
        application.techStack_ = normalizedTechnologies(draft.techStack_);
        application.notes_ = draft.notes_.trimmed();
        jobRepository_.insert(application);

        if (!database_.commit()) {
            throw std::runtime_error(database_.lastError().text().toStdString());
        }

        result.success_ = true;
        result.application_ = std::move(application);
        result.cvDocument_ = cvImport.document_;
        result.cvWasInserted_ = cvImport.wasInserted_;
    } catch (const std::exception& error) {
        database_.rollback();
        if (!copiedFilePath.isEmpty()) {
            QFile::remove(copiedFilePath);
        }
        result.message_ = QString::fromUtf8(error.what());
    }
    return result;
}

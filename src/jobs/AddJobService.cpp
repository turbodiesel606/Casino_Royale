#include "AddJobService.hpp"
#include "JobRepository.hpp"
#include "cvs/CvImportService.hpp"
#include "directory/CompanyRepository.hpp"

#include <QDate>
#include <QFile>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QUrl>
#include <QUuid>

namespace {

	QStringList normalizedTechStack(const QStringList& values)
	{
		// Normalizes the content entered in the Tech Stack field in the "+Add Job" window
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

	JobApplicationDraft buildJobApplicationDraft(const JobApplicationDraft& draft) {
		JobApplicationDraft tempDraft;
		tempDraft.jobTitle_ = draft.jobTitle_.trimmed();
		tempDraft.companyName_ = draft.companyName_.trimmed();
		tempDraft.status_ = draft.status_.trimmed().isEmpty() ? QStringLiteral("Applied") : draft.status_.trimmed();
		tempDraft.appliedDate_ = draft.appliedDate_.trimmed().isEmpty()
			? QDate::currentDate().toString(Qt::ISODate)
			: draft.appliedDate_.trimmed();
		tempDraft.jobUrl_ = draft.jobUrl_.trimmed();
		tempDraft.workFormat_ = draft.workFormat_;
		tempDraft.city_ = draft.city_;
		tempDraft.salary_ = draft.salary_;
		tempDraft.nextStep_ = draft.nextStep_;
		tempDraft.description_ = draft.description_;
		tempDraft.requirements_ = draft.requirements_;
		tempDraft.techStack_ = draft.techStack_;
		tempDraft.notes_ = draft.notes_;

		return tempDraft;
	}

	AddJobResult errorsCheck(const JobApplicationDraft& jADraft, const QUrl& selectedCvUrl) {

		AddJobResult result;
		if (jADraft.jobTitle_.isEmpty())
			result.fieldErrors_.insert(QStringLiteral("jobTitle"), QStringLiteral("Job title is required."));

		if (jADraft.companyName_.isEmpty())
			result.fieldErrors_.insert(QStringLiteral("companyName"), QStringLiteral("Company is required."));


		// Even date format is correct, 2026-99-87 is unacceptable.
		if (!QDate::fromString(jADraft.appliedDate_, Qt::ISODate).isValid())
			result.fieldErrors_.insert(QStringLiteral("appliedDate"), QStringLiteral("Use date format yyyy-MM-dd."));

		// Check whether the user selected a CV at all.
		if (selectedCvUrl.isEmpty())
			result.fieldErrors_.insert(QStringLiteral("cv"), QStringLiteral("Select a CV."));


		const auto jobUrl = jADraft.jobUrl_.trimmed();
		if (!jobUrl.isEmpty()) {
			const QUrl parsedUrl(jobUrl);
			if (!parsedUrl.isValid()
				|| (parsedUrl.scheme() != QStringLiteral("http") && parsedUrl.scheme() != QStringLiteral("https")))
			{
				result.fieldErrors_.insert(QStringLiteral("jobUrl"), QStringLiteral("Use a valid HTTP or HTTPS URL."));
			}
		}
		
		return result;
	}
	JobApplication buildJobApplication(
		const JobApplicationDraft& jADraft,
		const Company& company,
		const CvImportResult& cvImport)
	{
		JobApplication application;
		application.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
		application.companyId_ = company.id_;
		application.companyName_ = company.name_;
		application.companyInitials_ = company.name_.left(2).toUpper();
		application.companyAccent_ = QStringLiteral("#146ce0");
		application.jobTitle_ = jADraft.jobTitle_;
		application.jobUrl_ = jADraft.jobUrl_;
		application.workFormat_ = jADraft.workFormat_.trimmed();
		application.city_ = jADraft.city_.trimmed();
		application.salary_ = jADraft.salary_.trimmed();
		application.status_ = jADraft.status_;
		application.appliedDate_ = jADraft.appliedDate_;
		application.dateLabel_ = QDate::fromString(jADraft.appliedDate_, Qt::ISODate).toString(QStringLiteral("MMM d, yyyy"));
		application.nextStep_ = jADraft.nextStep_.trimmed();
		application.cvId_ = cvImport.document_.id_;
		application.cvFileName_ = cvImport.document_.originalFileName_;
		application.description_ = jADraft.description_.trimmed();
		application.requirements_ = jADraft.requirements_.trimmed();
		application.techStack_ = normalizedTechStack(jADraft.techStack_);
		application.notes_ = jADraft.notes_.trimmed();

		return application;
	}
}

AddJobService::AddJobService(
	QSqlDatabase& database,
	JobRepository& jobRepository,
	CompanyRepository& companyRepository,
	CvImportService& cvImportService)
	: database_(database)
	, jobRepository_(jobRepository)
	, companyRepository_(companyRepository)
	, cvImportService_(cvImportService)
{
}

AddJobResult AddJobService::create(const JobApplicationDraft& draft, const QUrl& selectedCvUrl) const
{
	// Normalization of Required Fields
	JobApplicationDraft jADraft = buildJobApplicationDraft(draft);
	AddJobResult addJobresult = errorsCheck(jADraft, selectedCvUrl);
	
	if (!addJobresult.fieldErrors_.isEmpty()) {
		addJobresult.message_ = QStringLiteral("Please correct the highlighted fields.");
		return addJobresult;
	}
	
	if (!database_.transaction()) {
		addJobresult.message_ = database_.lastError().text();
		return addJobresult;
	}

	QString copiedFilePath;
	try {
		const auto company = companyRepository_.findOrCreateByName(jADraft.companyName_);
		const auto cvImport = cvImportService_.importDocument(selectedCvUrl);
		copiedFilePath = cvImport.copiedFilePath_;

		JobApplication application = buildJobApplication(jADraft, company, cvImport);
		
		jobRepository_.insert(application);

		if (!database_.commit())
			throw std::runtime_error(database_.lastError().text().toStdString());

		addJobresult.success_ = true;
		addJobresult.application_ = std::move(application);
		addJobresult.company_ = company;
		addJobresult.cvDocument_ = cvImport.document_;
		addJobresult.cvWasInserted_ = cvImport.wasInserted_;
	}
	catch (const std::exception& error) {
		database_.rollback();
		if (!copiedFilePath.isEmpty()) {
			QFile::remove(copiedFilePath);
		}
		addJobresult.message_ = QString::fromUtf8(error.what());
	}
	return addJobresult;
}

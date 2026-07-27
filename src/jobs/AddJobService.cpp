#include "AddJobService.hpp"
#include "JobApplicationFactory.hpp"
#include "JobApplicationValidator.hpp"
#include "JobRepository.hpp"
#include "cvs/CvImportService.hpp"
#include "directory/CompanyRepository.hpp"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QUrl>

#include <stdexcept>

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
	const auto normalizedDraft = JobApplicationFactory::normalize(draft);
	const auto validation = JobApplicationValidator::validate(normalizedDraft, selectedCvUrl);
	AddJobResult result;
	result.fieldErrors_ = validation.fieldErrors_;
	if (!validation.isValid()) {
		result.message_ = QStringLiteral("Please correct the highlighted fields.");
		return result;
	}

	if (!database_.transaction()) {
		result.message_ = database_.lastError().text();
		return result;
	}

	QString copiedFilePath;
	try {
		const auto company = companyRepository_.findOrCreateByName(normalizedDraft.companyName_);
		const auto cvImport = cvImportService_.importDocument(selectedCvUrl);
		copiedFilePath = cvImport.copiedFilePath_;

		auto application = JobApplicationFactory::create(
			normalizedDraft,
			company,
			cvImport.document_);
		jobRepository_.insert(application);

		if (!database_.commit()) {
			throw std::runtime_error(database_.lastError().text().toStdString());
		}

		result.success_ = true;
		result.application_ = std::move(application);
		result.company_ = company;
		result.cvDocument_ = cvImport.document_;
		result.cvWasInserted_ = cvImport.wasInserted_;
	}
	catch (const std::exception& error) {
		database_.rollback();
		if (!copiedFilePath.isEmpty()) {
			QFile::remove(copiedFilePath);
		}
		result.message_ = QString::fromUtf8(error.what());
	}
	return result;
}

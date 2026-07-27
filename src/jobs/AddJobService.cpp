#include "AddJobService.hpp"
#include "JobApplicationFactory.hpp"
#include "JobApplicationValidator.hpp"
#include "JobRepository.hpp"
#include "cvs/CvImportService.hpp"
#include "directory/CompanyRepository.hpp"

#include <QSqlDatabase>
#include <QSqlError>
#include <QThread>
#include <QUrl>

#include <atomic>
#include <memory>
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
	, owningThread_(QThread::currentThread())
{
}

AddJobPreparationResult AddJobService::prepare(
    const JobApplicationDraft& draft,
    const QUrl& selectedCvUrl,
    const std::shared_ptr<std::atomic_bool>& cancellation) const
{
	const auto normalizedDraft = JobApplicationFactory::normalize(draft);
	const auto validation = JobApplicationValidator::validate(normalizedDraft, selectedCvUrl);
	AddJobPreparationResult result;
	result.draft_ = normalizedDraft;
	result.fieldErrors_ = validation.fieldErrors_;
	if (!validation.isValid()) {
		result.message_ = QStringLiteral("Please correct the highlighted fields.");
		return result;
	}

	const auto cvPreparation = cvImportService_.prepareDocument(selectedCvUrl, cancellation);
	result.cancelled_ = cvPreparation.cancelled_;
	result.message_ = cvPreparation.message_;
	result.cvPreparation_ = cvPreparation.preparation_;
	result.success_ = cvPreparation.succeeded();
	return result;
}

AddJobResult AddJobService::complete(AddJobPreparationResult preparation) const
{
	AddJobResult result;
	result.fieldErrors_ = preparation.fieldErrors_;
	if (!preparation.success_) {
		result.message_ = preparation.message_;
		return result;
	}
	if (QThread::currentThread() != owningThread_) {
		result.message_ = QStringLiteral("Job persistence must run on its owning thread.");
		return result;
	}

	if (!database_.transaction()) {
		result.message_ = database_.lastError().text();
		return result;
	}

	QString completedFilePath;
	try {
		const auto company = companyRepository_.findOrCreateByName(preparation.draft_.companyName_);
		const auto cvImport = cvImportService_.importPreparedDocument(preparation.cvPreparation_);
		completedFilePath = cvImport.completedFilePath_;

		auto application = JobApplicationFactory::create(
			preparation.draft_,
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
		result.message_ = QString::fromUtf8(error.what());
		if (!cvImportService_.removeCompletedFile(completedFilePath)) {
			result.message_.append(QStringLiteral(
				" The managed CV file could not be cleaned up and will be quarantined on restart."));
		}
	}
	return result;
}

AddJobResult AddJobService::create(
	const JobApplicationDraft& draft,
	const QUrl& selectedCvUrl) const
{
	auto cancellation = std::make_shared<std::atomic_bool>(false);
	return complete(prepare(draft, selectedCvUrl, cancellation));
}

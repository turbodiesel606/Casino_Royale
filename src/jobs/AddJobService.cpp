#include "AddJobService.hpp"
#include "JobApplicationFactory.hpp"
#include "JobApplicationValidator.hpp"
#include "JobRepository.hpp"
#include "cvs/CvImportService.hpp"
#include "directory/CompanyRepository.hpp"
#include "storage/SqlTransaction.hpp"

#include <QSqlDatabase>
#include <QThread>
#include <QUrl>

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
	const NormalizedJobApplicationDraft& draft,
	const QUrl& selectedCvUrl,
	const std::shared_ptr<CancellationState>& cancellation) const
{
	// Service - level defensive validation.
	const auto validation = JobApplicationValidator::validate(draft, selectedCvUrl);

	if (!validation.isValid()) {
		AddJobPreparationResult result;
		result.draft_ = draft;
		result.fieldErrors_ = validation.fieldErrors_;
		result.message_ = QStringLiteral("Please correct the highlighted fields.");
		return result;
	}

	return prepareValidated(draft, selectedCvUrl, cancellation);
}

AddJobPreparationResult AddJobService::prepareValidated(
	const NormalizedJobApplicationDraft& draft,
	const QUrl& selectedCvUrl,
	const std::shared_ptr<CancellationState>& cancellation) const
{
	// verify, copy, and SHA-256 hash the CV into a .part file.
	const auto cvPreparation =
		cvImportService_.prepareDocument(selectedCvUrl, cancellation);

	AddJobPreparationResult result;
	result.draft_ = draft;
	result.cancelled_ = cvPreparation.cancelled_;
	result.message_ = cvPreparation.message_;
	result.cvPreparation_ = cvPreparation.preparation_;
	result.success_ = cvPreparation.succeeded();

	return result;
}

AddJobResult AddJobService::complete(
	AddJobPreparationResult preparation,
	const std::shared_ptr<CancellationState>& cancellation) const
{
	AddJobResult result;
	result.fieldErrors_ = preparation.fieldErrors_;

	// Do not enter a transaction if CV preparation failed.
	if (!preparation.success_) {
		result.message_ = preparation.message_;
		return result;
	}

	if (QThread::currentThread() != owningThread_) {
		result.message_ = QStringLiteral("Job persistence must run on its owning thread.");
		return result;
	}

	QString completedFilePath;
	try {

		// Last zancellation check before transaction start.
		if (cancellation != nullptr && cancellation->isCancellationRequested()) {
			result.message_ = QStringLiteral("Job creation was canceled.");
			return result;
		}

		SqlTransaction transaction{database_, QStringLiteral("Add Job persistence")};
		const auto company = companyRepository_.findOrCreateByName(preparation.draft_.companyName_);
		const auto cvImport = cvImportService_.importPreparedDocument(
			preparation.cvPreparation_,
			CvArchivedDuplicatePolicy::PreserveArchived);
		completedFilePath = cvImport.completedFilePath_;

		auto application = JobApplicationFactory::create(
			preparation.draft_,
			company,
			cvImport.document_);
		jobRepository_.insert(application);

		transaction.commit();

		result.success_ = true;
		result.message_ = QStringLiteral("Job application saved successfully.");
		result.application_ = std::move(application);
		result.company_ = company;
		result.cvDocument_ = cvImport.document_;
		result.cvImportDisposition_ = cvImport.disposition_;
	}
	catch (const std::exception& error) {
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
	const auto preflightResult = JobApplicationValidator::preflight(
		draft,
		!selectedCvUrl.isEmpty());
	auto cancellation = std::make_shared<CancellationState>();
	return complete(
		prepareValidated(preflightResult.draft_, selectedCvUrl, cancellation),
		cancellation);
}

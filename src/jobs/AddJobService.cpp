#include "AddJobService.hpp"
#include "JobApplicationFactory.hpp"
#include "JobApplicationValidator.hpp"
#include "JobRepository.hpp"
#include "common/ExceptionUtils.hpp"
#include "cvs/CvImportService.hpp"
#include "cvs/CvLockWrapper.hpp"
#include "directory/CompanyRepository.hpp"
#include "storage/SqlTransaction.hpp"

#include <QSqlDatabase>
#include <QThread>
#include <QUrl>

#include <memory>
#include <exception>
#include <utility>

AddJobService::AddJobService(
	QSqlDatabase& database,
	JobRepository& jobRepository,
	CompanyRepository& companyRepository,
	CvImportService& cvImportService,
	CvLockWrapper& cvMutationQueue)
	: database_(database)
	, jobRepository_(jobRepository)
	, companyRepository_(companyRepository)
	, cvImportService_(cvImportService)
	, cvLock_{ cvMutationQueue }
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
	// Validate and buffer the CV; managed files are created only on an identity miss.
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
	
	std::unique_lock<std::mutex> cvLock{ cvLock_.getMutex() };

	try {
		if (cancellation != nullptr && cancellation->isCancellationRequested()){
			result.message_ = QStringLiteral("Job creation was canceled.");
			return result;
		}
		/*
		CV restoration / insertion, company resolution and job insertion
		must commit or roll back together.
		*/
		SqlTransaction transaction{ database_, QStringLiteral("Add Job persistence") };

		const auto cvImport = cvImportService_.importPreparedDocument(
			preparation.cvPreparation_,
			cancellation);
		if (!cvImport.success_) {
			result.message_ = cvImport.message_;
			return result;
		}

		// For now, this feature is not needed. Dont review it.
		const auto company = companyRepository_.findOrCreateByName(preparation.draft_.companyName_);
		
		auto application = JobApplicationFactory::create(
			preparation.draft_,
			company,
			cvImport.document_);

		jobRepository_.insert(application);

		result.message_ = QStringLiteral("Job application saved successfully.");
		result.application_ = std::move(application);
		result.company_ = company;
		result.cvDocument_ = cvImport.document_;
		result.cvImportDisposition_ = cvImport.disposition_;
		transaction.commit();
		result.success_ = true;
	}
	catch (...) {
		result.message_ = common::exceptionMessage(
			std::current_exception(), QStringLiteral("An unexpected Add Job error occurred."));
		// SqlTransaction has rolled back; retain the lease until cleanup finishes.
		if (preparation.cvPreparation_ != nullptr
			&& !cvImportService_.removeCompletedFile(preparation.cvPreparation_->finalFilePath_)) {
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
	if (!preflightResult.isValid()) {
		AddJobResult result;
		result.fieldErrors_ = preflightResult.fieldErrors_;
		result.message_ = preflightResult.message_;
		return result;
	}
	auto cancellation = std::make_shared<CancellationState>();
	return complete(
		prepareValidated(preflightResult.draft_, selectedCvUrl, cancellation),
		cancellation);
}

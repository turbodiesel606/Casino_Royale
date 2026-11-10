#include "AddJobService.hpp"
#include "JobApplicationFactory.hpp"
#include "JobApplicationValidator.hpp"
#include "JobRepository.hpp"
#include "common/ExceptionUtils.hpp"
#include "cvs/CvImportService.hpp"
#include "cvs/CvLockWrapper.hpp"
#include "directory/CompanyRepository.hpp"
#include "storage/SqlTransaction.hpp"
#include "storage/SqlQuery.hpp"
#include "cvs/CvPersistenceObservation.hpp"
#include "logging/ApplicationLog.hpp"

#include <QScopeGuard>

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
	const std::shared_ptr<CancellationState>& cancellation, quint64 operationId) const
{
	// Validate and buffer the CV; managed files are created only on an identity miss.
	const auto cvPreparation =
		cvImportService_.prepareDocument(selectedCvUrl, cancellation, LogFact::JobCreation, operationId);

	AddJobPreparationResult result;
	result.draft_ = draft;
	result.operationId_ = operationId;
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

	CvPersistenceObservation observation{LogFact::JobCreation, preparation.operationId_};
	if (QThread::currentThread() != owningThread_) {
        observation.invariant(LogFact::PersistWrongThread);
		result.message_ = QStringLiteral("Job persistence must run on its owning thread.");
		return result;
	}
	
	std::unique_lock<std::mutex> cvLock{ cvLock_.getMutex() };
    const auto report = qScopeGuard([&] {
        if (observation.reportFailure_) observation.recordFailure();
    });

	try {
		if (cancellation != nullptr && cancellation->isCancellationRequested()){
            observation.stopped(LogFact::PersistAfterLock);
			result.message_ = QStringLiteral("Job creation was canceled.");
			return result;
		}
		/*
		CV restoration / insertion, company resolution and job insertion
		must commit or roll back together.
		*/
		observation.stage_ = LogFact::PersistBegin;
		SqlTransaction transaction{ database_, QStringLiteral("Add Job persistence"),
            observation.operation_, &observation.rollback_, observation.operationId_ };

		const auto cvImport = cvImportService_.importPreparedDocument(
			preparation.cvPreparation_,
			cancellation, observation);
		if (!cvImport.success_) {
            observation.reportFailure_ = true;
			result.message_ = cvImport.message_;
			return result;
		}

		// For now, this feature is not needed. Dont review it.
		observation.stage_ = LogFact::PersistCompany;
		const auto company = companyRepository_.findOrCreateByName(preparation.draft_.companyName_);
		
		auto application = JobApplicationFactory::create(
			preparation.draft_,
			company,
			cvImport.document_);

		observation.stage_ = LogFact::PersistInsertJob;
		jobRepository_.insert(application, &observation.stage_);

		result.message_ = QStringLiteral("Job application saved successfully.");
		result.application_ = std::move(application);
		result.company_ = company;
		result.cvDocument_ = cvImport.document_;
		result.cvImportDisposition_ = cvImport.disposition_;
        observation.stage_ = LogFact::PersistCommit;
        const bool cancellationRequested = cancellationObservedBeforeCommit(cancellation);
        transaction.commit();
        result.success_ = true;
        const auto disposition = cvDispositionFact(result.cvImportDisposition_);
        if (cancellationRequested) {
            ApplicationLog::record(observation.operationId_, LogEventType::OperationCommittedAfterCancellationRequest,
                observation.operation_, disposition);
        } else {
            ApplicationLog::record(observation.operationId_, LogEventType::JobApplicationCreated, disposition);
        }
	}
	catch (...) {
        if (observation.cause_ == LogFact::CauseUnavailable)
            observation.cause_ = storage::sql::currentFailureCause();
        observation.reportFailure_ = true;
		result.message_ = common::exceptionMessage(
			std::current_exception(), QStringLiteral("An unexpected Add Job error occurred."));
		// SqlTransaction has rolled back; retain the lease until cleanup finishes.
		if (preparation.cvPreparation_ != nullptr) {
			cvImportService_.compensateCompletedFile(
				preparation.cvPreparation_->finalFilePath_, observation.cleanup_, result.message_);
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

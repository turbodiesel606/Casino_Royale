#include "UpdateJobService.hpp"

#include "JobApplicationFactory.hpp"
#include "JobApplicationValidator.hpp"
#include "JobRepository.hpp"
#include "common/CancellationState.hpp"
#include "common/ExceptionUtils.hpp"
#include "cvs/CvLockWrapper.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "directory/CompanyRepository.hpp"
#include "storage/SqlTransaction.hpp"
#include "storage/SqlQuery.hpp"
#include "cvs/CvPersistenceObservation.hpp"
#include "logging/ApplicationLog.hpp"

#include <QScopeGuard>

#include <QSqlDatabase>
#include <QThread>

#include <stdexcept>
#include <utility>

UpdateJobService::UpdateJobService(
	QSqlDatabase& database,
	JobRepository& jobRepository,
	CompanyRepository& companyRepository,
	CvImportService& cvImportService,
	CvLockWrapper& cvMutationQueue)
	: database_{ database }
	, jobRepository_{ jobRepository }
	, companyRepository_{ companyRepository }
	, cvImportService_{ cvImportService }
	, cvLock_{ cvMutationQueue }
	, owningThread_{ QThread::currentThread() }
{
}

UpdateJobPreparationResult UpdateJobService::prepare(
	const QString& applicationId,
	const NormalizedJobApplicationDraft& draft,
	const QUrl& replacementCvUrl,
	const std::shared_ptr<CancellationState>& cancellation) const
{
	return prepareImpl<DraftValidationMode::RequiredValidation>(
		applicationId,
		draft,
		replacementCvUrl,
		cancellation, 0);
}

UpdateJobPreparationResult UpdateJobService::prepareValidated(
	const QString& applicationId,
	const NormalizedJobApplicationDraft& draft,
	const QUrl& replacementCvUrl,
	const std::shared_ptr<CancellationState>& cancellation, quint64 operationId) const
{
	return prepareImpl<DraftValidationMode::AlreadyValidated>(
		applicationId,
		draft,
		replacementCvUrl,
		cancellation, operationId);
}

UpdateJobResult UpdateJobService::complete(
	UpdateJobPreparationResult preparation,
	const std::shared_ptr<CancellationState>& cancellation) const
{
	UpdateJobResult result;
	result.fieldErrors_ = preparation.fieldErrors_;
	if (!preparation.success_) {
		result.message_ = preparation.message_;
		return result;
	}
	CvPersistenceObservation observation{LogFact::JobUpdate, preparation.operationId_};
	if (QThread::currentThread() != owningThread_) {
        observation.invariant(LogFact::PersistWrongThread);
		result.message_ = QStringLiteral("Job update persistence must run on its owning thread.");
		return result;
	}
	if (cancellation != nullptr && cancellation->isCancellationRequested()) {
        observation.stopped(LogFact::PersistPrepare);
		result.message_ = QStringLiteral("The job update was canceled.");
		return result;
	}

	std::unique_lock<std::mutex> cvLock{ cvLock_.getMutex() };
    const auto report = qScopeGuard([&] {
        if (observation.reportFailure_) observation.recordFailure();
    });
	try {
		if (preparation.replacementCvRequested_) {
			if (cancellation != nullptr && cancellation->isCancellationRequested()) {
                observation.stopped(LogFact::PersistAfterLock);
				result.message_ = QStringLiteral("The job update was canceled.");
				return result;
			}
		}

		observation.stage_ = LogFact::PersistBegin;
		SqlTransaction transaction{ database_, QStringLiteral("Update Job persistence"),
            observation.operation_, &observation.rollback_, observation.operationId_ };

		std::optional<CvImportResult> cvImport;
		const CvDocument* replacementCv = nullptr;
		if (preparation.replacementCvRequested_) {
			cvImport = cvImportService_.importPreparedDocument(
				preparation.cvPreparation_,
				cancellation, observation);
			if (!cvImport->success_) {
            observation.reportFailure_ = true;
				result.message_ = cvImport->message_;
				return result;
			}
			replacementCv = &cvImport->document_;
		}
		observation.stage_ = LogFact::PersistCompany;
		const auto company = companyRepository_.findOrCreateByName(
			preparation.draft_.companyName_);

		auto application = JobApplicationFactory::update(
			preparation.existingApplication_,
			preparation.draft_,
			company,
			replacementCv);
		observation.stage_ = LogFact::PersistUpdateJob;
		if (!jobRepository_.update(application, &observation.stage_)) {
            observation.stage_ = LogFact::PersistLookupJob;
            observation.cause_ = LogFact::Missing;
			throw std::runtime_error("The job application no longer exists.");
		}

		result.message_ = QStringLiteral("Job application changes saved successfully.");
		result.previousCvId_ = preparation.existingApplication_.cvId_;
		result.application_ = std::move(application);
		result.company_ = company;
		if (cvImport) {
			result.replacementCvDocument_ = cvImport->document_;
			result.cvImportDisposition_ = cvImport->disposition_;
		}
        observation.stage_ = LogFact::PersistCommit;
        const bool cancellationRequested = cancellationObservedBeforeCommit(cancellation);
        transaction.commit();
        result.success_ = true;
        const bool replaced = preparation.replacementCvRequested_;
        const bool changed = result.previousCvId_ != result.application_.cvId_;
        const auto disposition = replaced ? cvDispositionFact(result.cvImportDisposition_) : LogFact::None;
        if (cancellationRequested) {
            ApplicationLog::record(observation.operationId_, LogEventType::OperationCommittedAfterCancellationRequest,
                observation.operation_, LogJobUpdate{.replacement_ = replaced,
                    .relationshipChanged_ = changed, .disposition_ = disposition});
        } else {
            ApplicationLog::record(observation.operationId_, LogEventType::JobApplicationUpdated,
                replaced ? LogFact::Yes : LogFact::No, changed ? LogFact::Yes : LogFact::No, disposition);
        }
	}
	catch (...) {
        if (observation.cause_ == LogFact::CauseUnavailable)
            observation.cause_ = storage::sql::currentFailureCause();
        observation.reportFailure_ = true;
		result.message_ = common::exceptionMessage(
			std::current_exception(), QStringLiteral("An unexpected job update error occurred."));
		// Rollback precedes file compensation, and the lease outlives both.
		if (preparation.cvPreparation_ != nullptr) {
			cvImportService_.compensateCompletedFile(
				preparation.cvPreparation_->finalFilePath_, observation.cleanup_, result.message_);
		}
	}
	return result;
}

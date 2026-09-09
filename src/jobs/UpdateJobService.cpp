#include "UpdateJobService.hpp"

#include "JobApplicationFactory.hpp"
#include "JobApplicationValidator.hpp"
#include "JobRepository.hpp"
#include "common/ExceptionUtils.hpp"
#include "cvs/CvLockWrapper.hpp"
#include "directory/CompanyRepository.hpp"
#include "storage/SqlTransaction.hpp"

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
		cancellation);
}

UpdateJobPreparationResult UpdateJobService::prepareValidated(
	const QString& applicationId,
	const NormalizedJobApplicationDraft& draft,
	const QUrl& replacementCvUrl,
	const std::shared_ptr<CancellationState>& cancellation) const
{
	return prepareImpl<DraftValidationMode::AlreadyValidated>(
		applicationId,
		draft,
		replacementCvUrl,
		cancellation);
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
	if (QThread::currentThread() != owningThread_) {
		result.message_ = QStringLiteral("Job update persistence must run on its owning thread.");
		return result;
	}
	if (cancellation != nullptr && cancellation->isCancellationRequested()) {
		result.message_ = QStringLiteral("The job update was canceled.");
		return result;
	}

	std::unique_lock<std::mutex> cvLock{ cvLock_.getMutex() };
	try {
		if (preparation.replacementCvRequested_) {
			if (cancellation != nullptr && cancellation->isCancellationRequested()) {
				result.message_ = QStringLiteral("The job update was canceled.");
				return result;
			}
		}

		SqlTransaction transaction{ database_, QStringLiteral("Update Job persistence") };

		std::optional<CvImportResult> cvImport;
		const CvDocument* replacementCv = nullptr;
		if (preparation.replacementCvRequested_) {
			cvImport = cvImportService_.importPreparedDocument(
				preparation.cvPreparation_,
				cancellation);
			if (!cvImport->success_) {
				result.message_ = cvImport->message_;
				return result;
			}
			replacementCv = &cvImport->document_;
		}
		const auto company = companyRepository_.findOrCreateByName(
			preparation.draft_.companyName_);

		auto application = JobApplicationFactory::update(
			preparation.existingApplication_,
			preparation.draft_,
			company,
			replacementCv);
		if (!jobRepository_.update(application)) {
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
		transaction.commit();
		result.success_ = true;
	}
	catch (...) {
		result.message_ = common::exceptionMessage(
			std::current_exception(), QStringLiteral("An unexpected job update error occurred."));
		// Rollback precedes file compensation, and the lease outlives both.
		if (preparation.cvPreparation_ != nullptr
			&& !cvImportService_.removeCompletedFile(preparation.cvPreparation_->finalFilePath_)) {
			result.message_.append(QStringLiteral(
				" The managed CV file could not be cleaned up and will be quarantined on restart."));
		}
	}
	return result;
}

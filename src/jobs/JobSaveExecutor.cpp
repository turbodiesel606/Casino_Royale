#include "JobSaveExecutor.hpp"

#include "JobApplicationValidator.hpp"
#include "JobRepository.hpp"
#include "common/ExceptionUtils.hpp"
#include "cvs/CvImportService.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "directory/CompanyRepository.hpp"
#include "storage/SqliteDatabase.hpp"
#include "storage/StoragePaths.hpp"

#include <QThread>

#include <exception>
#include <utility>

namespace {

	constexpr QLatin1String jobSaveError{ "An unexpected job-save worker error occurred." };

} // namespace

struct JobSaveExecutor::PipelineContext final
{
	PipelineContext(const QString& dataDirectory, CvLockWrapper& cvMutationQueue)
		//Resolve worker - local storage paths
		: storagePaths_{ dataDirectory }
		// Create a worker - thread SQLite connection.
		, database_{ storagePaths_.databasePath() }
		// Every repository wires with that same worker connection.
		, cvRepository_{ database_.connection() }
		, companyRepository_{ database_.connection() }
		, jobRepository_{ database_.connection() }
		, cvManagedFileStore_{ storagePaths_ }
		, cvImportService_{ cvManagedFileStore_, cvRepository_ }
		, addJobService_{
			database_.connection(),
			jobRepository_,
			companyRepository_,
			cvImportService_, cvMutationQueue }
			, updateJobService_{
				database_.connection(),
				jobRepository_,
				companyRepository_,
				cvImportService_, cvMutationQueue }
	{
	}

	StoragePaths storagePaths_;
	SqliteDatabase database_;
	CvRepository cvRepository_;
	CompanyRepository companyRepository_;
	JobRepository jobRepository_;
	CvManagedFileStore cvManagedFileStore_;
	CvImportService cvImportService_;
	AddJobService addJobService_;
	UpdateJobService updateJobService_;
};

JobSaveExecutor::JobSaveExecutor(QString dataDirectory, CvLockWrapper& cvLock)
	: dataDirectory_{ std::move(dataDirectory) }
	, cvLock_{ cvLock }
{
}

JobSaveExecutor::~JobSaveExecutor() = default;

AddJobSaveOutcome JobSaveExecutor::process(AddJobRequest request)
{
	Q_ASSERT(QThread::currentThread() == thread());
	// Reject a malformed request that lacks the required shared
	// cancellation/correlation identity.
	if (request.cancellation_ == nullptr) {
		return saveOutcome(
			request,
			addFailure(QStringLiteral(
				"The Add Job cancellation state is unavailable.")));
	}

	// Fast cooperative-cancellation check before validation or lazy storage initialization.
	if (request.cancellation_->isCancellationRequested()) {
		return saveOutcome(
			request,
			addFailure(QStringLiteral("Job creation was canceled.")));
	}

	PipelineContext* context = nullptr;
	try {
		/* Note: PipelineContext can only be created within this function(in this thread)
	and remains in existence until the program ends. */
		context = &ensureContext();
	}
	catch (...) {
		return saveOutcome(
			request,
			addFailure(common::exceptionMessage(
				std::current_exception(),
				jobSaveError)));
	}

	AddJobResult result;
	try {
		// Result of prepareValidated(...) should be prvalue for optimisation
		// because of AddJobPreparationResult.
		// Do not try to store the result in a temporary variable.
		result = context->addJobService_.complete(
			context->addJobService_.prepareValidated(
				request.draft_,
				request.selectedCvUrl_,
				request.cancellation_),
			request.cancellation_);

	}
	catch (...) {
		result.message_ = common::exceptionMessage(
			std::current_exception(),
			jobSaveError);
	}

	return saveOutcome(request, std::move(result));
}

UpdateJobSaveOutcome JobSaveExecutor::process(UpdateJobRequest request)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (request.cancellation_ == nullptr) {
		return updateOutcome(
			request,
			updateFailure(QStringLiteral(
				"The job update cancellation state is unavailable.")));
	}

	if (request.cancellation_->isCancellationRequested()) {
		return updateOutcome(
			request,
			updateFailure(QStringLiteral("The job update was canceled.")));
	}

	PipelineContext* context = nullptr;
	try {
		/* Note: PipelineContext can only be created within this function(in this thread)
		and remains in existence until the program ends. */
		context = &ensureContext();
	}
	catch (...) {
		return updateOutcome(
			request,
			updateFailure(common::exceptionMessage(
				std::current_exception(),
				jobSaveError)));
	}

	UpdateJobResult result;
	try {
		/* Result of prepareValidated(...) should be prvalue for optimisation
		 because of AddJobPreparationResult.
		Do not try to store the result in a temporary variable.
		*/
		result = context->updateJobService_.complete(
			context->updateJobService_.prepareValidated(
				request.applicationId_,
				request.draft_,
				request.replacementCvUrl_,
				request.cancellation_),
			request.cancellation_);
	}
	catch (...) {
		result.message_ = common::exceptionMessage(
			std::current_exception(),
			jobSaveError);
	}

	return updateOutcome(request, std::move(result));
}

void JobSaveExecutor::destroyContext()
{
	Q_ASSERT(QThread::currentThread() == thread());
	context_.reset();
}

JobSaveExecutor::PipelineContext& JobSaveExecutor::ensureContext()
{
	if (context_ == nullptr)
		context_ = std::make_unique<PipelineContext>(dataDirectory_, cvLock_);

	return *context_;
}

AddJobResult JobSaveExecutor::addFailure(
	QString message,
	QVariantMap fieldErrors)
{
	AddJobResult result;
	result.fieldErrors_ = std::move(fieldErrors);
	result.message_ = std::move(message);
	return result;
}

UpdateJobResult JobSaveExecutor::updateFailure(
	QString message,
	QVariantMap fieldErrors)
{
	UpdateJobResult result;
	result.fieldErrors_ = std::move(fieldErrors);
	result.message_ = std::move(message);
	return result;
}

AddJobSaveOutcome JobSaveExecutor::saveOutcome(
	const AddJobRequest& request,
	AddJobResult result)
{
	return {
		request.operationId_,
		request.cancellation_,
		request.draft_.jobTitle_,
		std::move(result) };
}

UpdateJobSaveOutcome JobSaveExecutor::updateOutcome(
	const UpdateJobRequest& request,
	UpdateJobResult result)
{
	return {
		request.operationId_,
		request.cancellation_,
		request.applicationId_,
		request.draft_.jobTitle_,
		std::move(result) };
}

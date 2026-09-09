#include "CvImportExecutor.hpp"

#include "CvImportService.hpp"
#include "CvManagedFileStore.hpp"
#include "cvs/CvLockWrapper.hpp"
#include "CvRepository.hpp"
#include "common/ExceptionUtils.hpp"
#include "storage/SqliteDatabase.hpp"
#include "storage/SqlTransaction.hpp"
#include "storage/StoragePaths.hpp"

#include <QFileInfo>
#include <QThread>

#include <exception>
#include <utility>

struct CvImportExecutor::PipelineContext final
{
	explicit PipelineContext(const QString& dataDirectory)
		: storagePaths_{ dataDirectory }
		, database_{ storagePaths_.databasePath() }
		, repository_{ database_.connection() }
		, managedFileStore_{ storagePaths_ }
		, importService_{ managedFileStore_, repository_ }
	{
	}

	StoragePaths storagePaths_;
	SqliteDatabase database_;
	CvRepository repository_;
	CvManagedFileStore managedFileStore_;
	CvImportService importService_;
};

CvImportExecutor::CvImportExecutor(QString dataDirectory, CvLockWrapper& cvMutationQueue)
	: dataDirectory_{ std::move(dataDirectory) }
	, cvLock_{ cvMutationQueue }
{
}

CvImportExecutor::~CvImportExecutor() = default;

CvImportSaveOutcome CvImportExecutor::process(CvImportRequest request)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (request.cancellation_ == nullptr) {
		return failureOutcome(
			request,
			QStringLiteral(
				"The CV import cancellation state is unavailable."));
	}

	if (request.cancellation_->isCancellationRequested()) {
		return failureOutcome(
			request,
			QStringLiteral("CV import was canceled."));
	}

	PipelineContext* context = nullptr;
	try {
		context = &ensureContext();
	}
	catch (...) {
		return failureOutcome(
			request,
			importExceptionMessage(std::current_exception()));
	}

	CvManagedFilePreparationResult preparation;
	try {
		preparation = context->importService_.prepareDocument(
			request.sourceUrl_,
			request.cancellation_);
	}
	catch (...) {
		return failureOutcome(
			request,
			importExceptionMessage(std::current_exception()));
	}

	if (!preparation.succeeded()) {
		const auto message = preparation.cancelled_
			? QStringLiteral("CV import was canceled.")
			: (preparation.message_.isEmpty()
				? QStringLiteral("The CV could not be prepared for import.")
				: preparation.message_);
		preparation.preparation_.reset();
		return failureOutcome(request, message);
	}

	if (request.cancellation_->isCancellationRequested()) {
		preparation.preparation_.reset();
		return failureOutcome(
			request,
			QStringLiteral("CV import was canceled."));
	}

	std::unique_lock<std::mutex> cvLock{ cvLock_.getMutex() };
	try {
		if (request.cancellation_ != nullptr && request.cancellation_->isCancellationRequested()) {
			preparation.preparation_.reset();
			return failureOutcome(request, QStringLiteral("CV import was canceled."));
		}

		SqlTransaction transaction{
			context->database_.connection(), QStringLiteral("CV import persistence") };
		auto result = context->importService_.importPreparedDocument(
			preparation.preparation_,
			request.cancellation_);

		if (!result.success_)
			return failureOutcome(request, result.message_);
		
		CvImportSaveOutcome outcome;
		outcome.operationId_ = request.operationId_;
		outcome.cancellation_ = request.cancellation_;
		outcome.fileName_ = fileNameForUrl(request.sourceUrl_);
		outcome.document_ = std::move(result.document_);
		outcome.success_ = true;
		outcome.disposition_ = result.disposition_;
		outcome.message_ = cvImportSuccessMessage(result.disposition_);
		transaction.commit();
		return outcome;
	}
	catch (...) {
		auto message = importExceptionMessage(std::current_exception());
		// The transaction has rolled back, but no later CV ticket can run yet.
		if (!context->importService_.removeCompletedFile(preparation.preparation_->finalFilePath_)) {
			message.append(QStringLiteral(
				" The managed CV file could not be cleaned up and will be quarantined on restart."));
		}
		return failureOutcome(request, std::move(message));
	}
}

void CvImportExecutor::destroyContext()
{
	Q_ASSERT(QThread::currentThread() == thread());
	context_.reset();
}

CvImportExecutor::PipelineContext& CvImportExecutor::ensureContext()
{
	if (context_ == nullptr)
		context_ = std::make_unique<PipelineContext>(dataDirectory_);

	return *context_;
}

QString CvImportExecutor::fileNameForUrl(const QUrl& sourceUrl)
{
	const auto fileName = QFileInfo{ sourceUrl.toLocalFile() }.fileName();
	return fileName.isEmpty()
		? QStringLiteral("Selected CV")
		: fileName;
}

QString CvImportExecutor::importExceptionMessage(
	const std::exception_ptr& exception)
{
	return common::exceptionMessage(
		exception,
		QStringLiteral("An unexpected CV import error occurred."));
}

CvImportSaveOutcome CvImportExecutor::failureOutcome(
	const CvImportRequest& request,
	QString message)
{
	CvImportSaveOutcome outcome;
	outcome.operationId_ = request.operationId_;
	outcome.cancellation_ = request.cancellation_;
	outcome.fileName_ = fileNameForUrl(request.sourceUrl_);
	outcome.message_ = std::move(message);
	return outcome;
}

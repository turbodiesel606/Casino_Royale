#include "CvImportWorker.hpp"

#include "CvImportService.hpp"
#include "CvManagedFileStore.hpp"
#include "CvRepository.hpp"
#include "storage/SqliteDatabase.hpp"
#include "storage/StoragePaths.hpp"

#include <QFileInfo>
#include <QMetaObject>
#include <QThread>

#include <exception>
#include <memory>
#include <utility>

namespace {

	QString fileNameForUrl(const QUrl& sourceUrl)
	{
		const auto fileName = QFileInfo{ sourceUrl.toLocalFile() }.fileName();
		return fileName.isEmpty() ? QStringLiteral("Selected CV") : fileName;
	}

	QString exceptionMessage(const std::exception_ptr& exception)
	{
		try {
			if (exception != nullptr) {
				std::rethrow_exception(exception);
			}
		}
		catch (const std::exception& error) {
			const auto message = QString::fromUtf8(error.what());
			if (!message.isEmpty()) {
				return message;
			}
		}
		catch (...) {
		}
		return QStringLiteral("An unexpected CV import error occurred.");
	}

	CvImportSaveOutcome failureOutcome(
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

}

class CvImportWorker::Executor final : public QObject
{
public:
	explicit Executor(QString dataDirectory)
		: dataDirectory_{ std::move(dataDirectory) }
	{
	}

	void process(CvImportRequest request, CvImportWorker* facade)
	{
		Q_ASSERT(QThread::currentThread() == thread());

		if (request.cancellation_ == nullptr) {
			postImport(
				facade,
				failureOutcome(
					request,
					QStringLiteral("The CV import cancellation state is unavailable.")));
			return;
		}

		if (request.cancellation_->isCancellationRequested()) {
			postImport(
				facade,
				failureOutcome(request, QStringLiteral("CV import was canceled.")));
			return;
		}
		//std::this_thread::sleep_for(std::chrono::seconds{ 5 });
		PipelineContext* context = nullptr;
		try {
			context = &ensureContext();
		}
		catch (...) {
			postImport(
				facade,
				failureOutcome(request, exceptionMessage(std::current_exception())));
			return;
		}

		CvManagedFilePreparationResult preparation;

		try {
			preparation = context->importService_.prepareDocument(
				request.sourceUrl_,
				request.cancellation_);
		}
		catch (...) {
			postImport(
				facade,
				failureOutcome(request, exceptionMessage(std::current_exception())));
			return;
		}

		if (!preparation.succeeded()) {
			const auto message = preparation.cancelled_
				? QStringLiteral("CV import was canceled.")
				: (preparation.message_.isEmpty()
					? QStringLiteral("The CV could not be prepared for import.")
					: preparation.message_);
			preparation.preparation_.reset();
			postImport(facade, failureOutcome(request, message));
			return;
		}

		if (request.cancellation_->isCancellationRequested()) {
			preparation.preparation_.reset();
			postImport(
				facade,
				failureOutcome(request, QStringLiteral("CV import was canceled.")));
			return;
		}

		try {
			auto result = context->importService_.importPreparedDocument(
				preparation.preparation_,
				CvArchivedDuplicatePolicy::RestoreArchived);
			preparation.preparation_.reset();
			CvImportSaveOutcome outcome;
			outcome.operationId_ = request.operationId_;
			outcome.cancellation_ = request.cancellation_;
			outcome.fileName_ = fileNameForUrl(request.sourceUrl_);
			outcome.document_ = std::move(result.document_);
			outcome.success_ = true;
			outcome.disposition_ = result.disposition_;
			if (result.disposition_ == CvImportDisposition::Inserted) {
				outcome.message_ = QStringLiteral("CV added successfully.");
			} else if (result.disposition_ == CvImportDisposition::RestoredArchived) {
				outcome.message_ = QStringLiteral("The archived CV was restored to the library.");
			} else {
				outcome.message_ = QStringLiteral(
					"A CV with the same filename and SHA-256 already exists.");
			}

			postImport(facade, std::move(outcome));

		}
		catch (...) {
			preparation.preparation_.reset();
			postImport(
				facade,
				failureOutcome(request, exceptionMessage(std::current_exception())));
		}
	}

	void destroyContext()
	{
		Q_ASSERT(QThread::currentThread() == thread());
		context_.reset();
	}

private:
	struct PipelineContext final
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

	PipelineContext& ensureContext()
	{
		if (context_ == nullptr) {
			context_ = std::make_unique<PipelineContext>(dataDirectory_);
		}
		return *context_;
	}

	static void postImport(CvImportWorker* facade, CvImportSaveOutcome outcome)
	{
		QMetaObject::invokeMethod(
			facade,
			[facade, outcome = std::move(outcome)]() mutable {
				facade->deliverImportOutcome(std::move(outcome));
			},
			Qt::QueuedConnection);
	}

	QString dataDirectory_;
	std::unique_ptr<PipelineContext> context_;
};

CvImportWorker::CvImportWorker(QString dataDirectory, QObject* parent)
	: QObject{ parent }
	, dataDirectory_{ std::move(dataDirectory) }
{
	workerThread_.setObjectName(QStringLiteral("CvImportWorkerThread"));
	qRegisterMetaType<CvImportSaveOutcome>();
}

CvImportWorker::~CvImportWorker()
{
	shutdown();
}

void CvImportWorker::submit(CvImportRequest request)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (shuttingDown_) {
		queueUnavailableOutcome(
			request,
			QStringLiteral("The CV import worker is shutting down."));
		return;
	}
	if (busy_) {
		queueUnavailableOutcome(
			request,
			QStringLiteral("The CV import worker already has an active request."));
		return;
	}

	if (executor_ == nullptr) {
		executor_ = new Executor{ dataDirectory_ };
		executor_->moveToThread(&workerThread_);
		connect(
			&workerThread_,
			&QThread::finished,
			executor_,
			&QObject::deleteLater);
		workerThread_.start();
	}

	activeOperationId_ = request.operationId_;
	activeCancellation_ = request.cancellation_;
	busy_ = true;

	auto* const executor = executor_;
	QMetaObject::invokeMethod(
		executor,
		[executor, request = std::move(request), this]() mutable {
			executor->process(std::move(request), this);
		},
		Qt::QueuedConnection);
}

void CvImportWorker::shutdown()
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (shuttingDown_) {
		return;
	}
	shuttingDown_ = true;

	if (activeCancellation_ != nullptr) {
		activeCancellation_->requestCancellation();
	}

	if (executor_ != nullptr) {
		if (!workerThread_.isRunning()) {
			workerThread_.start();
		}

		QMetaObject::invokeMethod(
			executor_,
			[executor = executor_]() {
				executor->destroyContext();
			},
			Qt::BlockingQueuedConnection);

		workerThread_.quit();
		workerThread_.wait();
		executor_ = nullptr;
	}

	clearActiveRequest();
}

bool CvImportWorker::isRunning() const
{
	return workerThread_.isRunning();
}

void CvImportWorker::deliverImportOutcome(CvImportSaveOutcome outcome)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (shuttingDown_
		|| !isActiveOutcome(outcome.operationId_, outcome.cancellation_)) {
		return;
	}

	clearActiveRequest();
	emit importCompleted(outcome);
}

void CvImportWorker::queueUnavailableOutcome(
	const CvImportRequest& request,
	const QString& message)
{
	auto outcome = failureOutcome(request, message);
	QMetaObject::invokeMethod(
		this,
		[this, outcome = std::move(outcome)]() mutable {
			emit importCompleted(outcome);
		},
		Qt::QueuedConnection);
}

bool CvImportWorker::isActiveOutcome(
	quint64 operationId,
	const std::shared_ptr<CancellationState>& cancellation) const
{
	return busy_
		&& activeOperationId_ == operationId
		&& activeCancellation_ == cancellation;
}

void CvImportWorker::clearActiveRequest()
{
	busy_ = false;
	activeOperationId_ = 0;
	activeCancellation_.reset();
}

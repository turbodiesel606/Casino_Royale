#include "AddJobWorker.hpp"

#include "JobApplicationValidator.hpp"
#include "JobRepository.hpp"
#include "cvs/CvImportService.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "directory/CompanyRepository.hpp"
#include "storage/SqliteDatabase.hpp"
#include "storage/StoragePaths.hpp"

#include <QMetaObject>
#include <QThread>

#include <exception>
#include <memory>
#include <utility>

namespace {

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
		return QStringLiteral("An unexpected Add Job worker error occurred.");
	}

}

class AddJobWorker::Executor final : public QObject
{
public:
	explicit Executor(QString dataDirectory)
		: dataDirectory_{ std::move(dataDirectory) }
	{
	}

	void process(AddJobRequest request, AddJobWorker* facade)
	{  // this function process 2 or more validations for field and file. 
	   // Maybe it is redundant...?

		// check whether this foo called from correct thread
		Q_ASSERT(QThread::currentThread() == thread());

		if (request.cancellation_ == nullptr) {
			postSave(
				facade,
				saveOutcome(
					request,
					failureResult(QStringLiteral(
						"The Add Job cancellation state is unavailable."))));
			return;
		}

		if (request.cancellation_->isCancellationRequested()) {
			postSave(
				facade,
				saveOutcome(
					request,
					failureResult(QStringLiteral("Job creation was canceled."))));
			return;
		}

		JobApplicationValidationResult validation;
		try {
			validation = JobApplicationValidator::validate(
				request.draft_,
				request.selectedCvUrl_);
		}
		catch (...) {
			postSave(
				facade,
				saveOutcome(
					request,
					failureResult(exceptionMessage(std::current_exception()))));
			return;
		}

		if (!validation.isValid()) {
			postSave(
				facade,
				saveOutcome(
					request,
					failureResult(
						QStringLiteral("Please correct the highlighted fields."),
						validation.fieldErrors_)));
			return;
		}

		if (request.cancellation_->isCancellationRequested()) {// After defensive validation.
			postSave(
				facade,
				saveOutcome(
					request,
					failureResult(QStringLiteral("Job creation was canceled."))));
			return;
		}

		PipelineContext* context = nullptr;
		try {
			context = &ensureContext();
		}
		catch (...) {
			postSave(
				facade,
				saveOutcome(
					request,
					failureResult(exceptionMessage(std::current_exception()))));
			return;
		}

		if (request.cancellation_->isCancellationRequested()) { // After context initialization.
			postSave(
				facade,
				saveOutcome(
					request,
					failureResult(QStringLiteral("Job creation was canceled."))));
			return;
		}

		AddJobResult result;
		try {
			auto preparation = context->addJobService_.prepare(
				request.draft_,
				request.selectedCvUrl_,
				request.cancellation_);
			result = context->addJobService_.complete(
				std::move(preparation),
				request.cancellation_);
		}
		catch (...) {
			result.message_ = exceptionMessage(std::current_exception());
		}

		postSave(
			facade,
			saveOutcome(request, std::move(result)));

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
			, cvRepository_{ database_.connection() }
			, companyRepository_{ database_.connection() }
			, jobRepository_{ database_.connection() }
			, cvManagedFileStore_{ storagePaths_ }
			, cvImportService_{ cvManagedFileStore_, cvRepository_ }
			, addJobService_{
				  database_.connection(),
				  jobRepository_,
				  companyRepository_,
				  cvImportService_ }
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
	};

	PipelineContext& ensureContext()
	{
		if (context_ == nullptr)
			context_ = std::make_unique<PipelineContext>(dataDirectory_);

		return *context_;
	}

	static AddJobResult failureResult(
		QString message,
		QVariantMap fieldErrors = {})
	{
		AddJobResult result;
		result.fieldErrors_ = std::move(fieldErrors);
		result.message_ = std::move(message);
		return result;
	}

	static AddJobSaveOutcome saveOutcome(
		const AddJobRequest& request,
		AddJobResult result)
	{
		return {
			request.operationId_,
			request.cancellation_,
			request.draft_.jobTitle_,
			std::move(result) };
	}

	static void postSave(AddJobWorker* facade, AddJobSaveOutcome outcome)
	{
		QMetaObject::invokeMethod(
			facade,
			[facade, outcome = std::move(outcome)]() mutable {
				facade->deliverSaveOutcome(std::move(outcome));
			},
			Qt::QueuedConnection);
	}

	QString dataDirectory_;
	std::unique_ptr<PipelineContext> context_;
};

AddJobWorker::AddJobWorker(QString dataDirectory, QObject* parent)
	: QObject{ parent }
	, dataDirectory_{ std::move(dataDirectory) }
{
	workerThread_.setObjectName(QStringLiteral("AddJobWorkerThread"));
	qRegisterMetaType<AddJobSaveOutcome>();
}

AddJobWorker::~AddJobWorker()
{
	shutdown();
}

void AddJobWorker::submit(AddJobRequest request)
{
	// ?
	//Q_ASSERT(QThread::currentThread() == thread());
	if (shuttingDown_) {
		queueUnavailableOutcome(
			request,
			QStringLiteral("The Add Job worker is shutting down."));
		return;
	}
	if (busy_) {
		queueUnavailableOutcome(
			request,
			QStringLiteral("The Add Job worker already has an active request."));
		return;
	}

	if (executor_ == nullptr) {
		executor_ = new Executor{ dataDirectory_ };
		// Move the executor's thread affinity to the worker thread so queued calls 
		// targeting the executor are processed by the worker thread's event loop.    
		executor_->moveToThread(&workerThread_);
		// Schedule the executor for deletion when the worker thread finishes.
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

	// Queue the processing task in the executor's thread. 
	// Because the executor belongs to workerThread_, the lambda will execute there 
	// asynchronously when the worker thread's event loop processes the queued call.
	QMetaObject::invokeMethod(
		executor,
		[executor, request = std::move(request), this]() mutable {
			executor->process(std::move(request), this);
		},
		Qt::QueuedConnection);
}

void AddJobWorker::shutdown()
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

bool AddJobWorker::isRunning() const
{
	return workerThread_.isRunning();
}

void AddJobWorker::deliverSaveOutcome(AddJobSaveOutcome outcome)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (shuttingDown_
		|| !isActiveOutcome(outcome.operationId_, outcome.cancellation_)) {
		return;
	}

	clearActiveRequest();
	emit saveCompleted(outcome);
}

void AddJobWorker::queueUnavailableOutcome(
	const AddJobRequest& request,
	const QString& message)
{
	AddJobResult result;
	result.message_ = message;
	AddJobSaveOutcome outcome{
		request.operationId_,
		request.cancellation_,
		request.draft_.jobTitle_,
		std::move(result) };
	QMetaObject::invokeMethod(
		this,
		[this, outcome = std::move(outcome)]() mutable {
			emit saveCompleted(outcome);
		},
		Qt::QueuedConnection);
}

bool AddJobWorker::isActiveOutcome(
	quint64 operationId,
	const std::shared_ptr<CancellationState>& cancellation) const
{
	return busy_
		&& activeOperationId_ == operationId
		&& activeCancellation_ == cancellation;
}

void AddJobWorker::clearActiveRequest()
{
	busy_ = false;
	activeOperationId_ = 0;
	activeCancellation_.reset();
}

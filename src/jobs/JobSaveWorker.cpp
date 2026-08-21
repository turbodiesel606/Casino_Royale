#include "JobSaveWorker.hpp"

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
    return QStringLiteral("An unexpected job-save worker error occurred.");
}

} // namespace

class JobSaveWorker::Executor final : public QObject
{
public:
    explicit Executor(QString dataDirectory)
        : dataDirectory_{std::move(dataDirectory)}
    {
    }

    void process(AddJobRequest request, JobSaveWorker* facade)
    {
        Q_ASSERT(QThread::currentThread() == thread());

        if (request.cancellation_ == nullptr) {
            postSave(
                facade,
                saveOutcome(
                    request,
                    addFailure(QStringLiteral(
                        "The Add Job cancellation state is unavailable."))));
            return;
        }
        if (request.cancellation_->isCancellationRequested()) {
            postSave(
                facade,
                saveOutcome(
                    request,
                    addFailure(QStringLiteral("Job creation was canceled."))));
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
                    addFailure(exceptionMessage(std::current_exception()))));
            return;
        }
        if (!validation.isValid()) {
            postSave(
                facade,
                saveOutcome(
                    request,
                    addFailure(
                        QStringLiteral("Please correct the highlighted fields."),
                        validation.fieldErrors_)));
            return;
        }
        if (request.cancellation_->isCancellationRequested()) {
            postSave(
                facade,
                saveOutcome(
                    request,
                    addFailure(QStringLiteral("Job creation was canceled."))));
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
                    addFailure(exceptionMessage(std::current_exception()))));
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

        postSave(facade, saveOutcome(request, std::move(result)));
    }

    void process(UpdateJobRequest request, JobSaveWorker* facade)
    {
        Q_ASSERT(QThread::currentThread() == thread());

        if (request.cancellation_ == nullptr) {
            postUpdate(
                facade,
                updateOutcome(
                    request,
                    updateFailure(QStringLiteral(
                        "The job update cancellation state is unavailable."))));
            return;
        }
        if (request.cancellation_->isCancellationRequested()) {
            postUpdate(
                facade,
                updateOutcome(
                    request,
                    updateFailure(QStringLiteral("The job update was canceled."))));
            return;
        }

        PipelineContext* context = nullptr;
        try {
            context = &ensureContext();
        }
        catch (...) {
            postUpdate(
                facade,
                updateOutcome(
                    request,
                    updateFailure(exceptionMessage(std::current_exception()))));
            return;
        }

        UpdateJobResult result;
        try {
            auto preparation = context->updateJobService_.prepare(
                request.applicationId_,
                request.draft_,
                request.replacementCvUrl_,
                request.cancellation_);
            result = context->updateJobService_.complete(
                std::move(preparation),
                request.cancellation_);
        }
        catch (...) {
            result.message_ = exceptionMessage(std::current_exception());
        }

        postUpdate(facade, updateOutcome(request, std::move(result)));
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
            : storagePaths_{dataDirectory}
            , database_{storagePaths_.databasePath()}
            , cvRepository_{database_.connection()}
            , companyRepository_{database_.connection()}
            , jobRepository_{database_.connection()}
            , cvManagedFileStore_{storagePaths_}
            , cvImportService_{cvManagedFileStore_, cvRepository_}
            , addJobService_{
                  database_.connection(),
                  jobRepository_,
                  companyRepository_,
                  cvImportService_}
            , updateJobService_{
                  database_.connection(),
                  jobRepository_,
                  companyRepository_,
                  cvImportService_}
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

    PipelineContext& ensureContext()
    {
        if (context_ == nullptr) {
            context_ = std::make_unique<PipelineContext>(dataDirectory_);
        }
        return *context_;
    }

    static AddJobResult addFailure(QString message, QVariantMap fieldErrors = {})
    {
        AddJobResult result;
        result.fieldErrors_ = std::move(fieldErrors);
        result.message_ = std::move(message);
        return result;
    }

    static UpdateJobResult updateFailure(QString message, QVariantMap fieldErrors = {})
    {
        UpdateJobResult result;
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
            std::move(result)};
    }

    static UpdateJobSaveOutcome updateOutcome(
        const UpdateJobRequest& request,
        UpdateJobResult result)
    {
        return {
            request.operationId_,
            request.cancellation_,
            request.applicationId_,
            request.draft_.jobTitle_,
            std::move(result)};
    }

    static void postSave(JobSaveWorker* facade, AddJobSaveOutcome outcome)
    {
        QMetaObject::invokeMethod(
            facade,
            [facade, outcome = std::move(outcome)]() mutable {
                facade->deliverSaveOutcome(std::move(outcome));
            },
            Qt::QueuedConnection);
    }

    static void postUpdate(JobSaveWorker* facade, UpdateJobSaveOutcome outcome)
    {
        QMetaObject::invokeMethod(
            facade,
            [facade, outcome = std::move(outcome)]() mutable {
                facade->deliverUpdateOutcome(std::move(outcome));
            },
            Qt::QueuedConnection);
    }

    QString dataDirectory_;
    std::unique_ptr<PipelineContext> context_;
};

JobSaveWorker::JobSaveWorker(QString dataDirectory, QObject* parent)
    : QObject{parent}
    , dataDirectory_{std::move(dataDirectory)}
{
    workerThread_.setObjectName(QStringLiteral("JobSaveWorkerThread"));
    qRegisterMetaType<AddJobSaveOutcome>();
    qRegisterMetaType<UpdateJobSaveOutcome>();
}

JobSaveWorker::~JobSaveWorker()
{
    shutdown();
}

void JobSaveWorker::submit(AddJobRequest request)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (shuttingDown_) {
        queueUnavailableOutcome(
            request,
            QStringLiteral("The job-save worker is shutting down."));
        return;
    }
    if (busy_) {
        queueUnavailableOutcome(
            request,
            QStringLiteral("The job-save worker already has an active request."));
        return;
    }

    if (executor_ == nullptr) {
        executor_ = new Executor{dataDirectory_};
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

void JobSaveWorker::submit(UpdateJobRequest request)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (shuttingDown_) {
        queueUnavailableOutcome(
            request,
            QStringLiteral("The job-save worker is shutting down."));
        return;
    }
    if (busy_) {
        queueUnavailableOutcome(
            request,
            QStringLiteral("The job-save worker already has an active request."));
        return;
    }

    if (executor_ == nullptr) {
        executor_ = new Executor{dataDirectory_};
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

void JobSaveWorker::shutdown()
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

bool JobSaveWorker::isRunning() const
{
    return workerThread_.isRunning();
}

void JobSaveWorker::deliverSaveOutcome(AddJobSaveOutcome outcome)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (shuttingDown_
        || !isActiveOutcome(outcome.operationId_, outcome.cancellation_)) {
        return;
    }

    clearActiveRequest();
    emit saveCompleted(outcome);
}

void JobSaveWorker::deliverUpdateOutcome(UpdateJobSaveOutcome outcome)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (shuttingDown_
        || !isActiveOutcome(outcome.operationId_, outcome.cancellation_)) {
        return;
    }

    clearActiveRequest();
    emit updateCompleted(outcome);
}

void JobSaveWorker::queueUnavailableOutcome(
    const AddJobRequest& request,
    const QString& message)
{
    AddJobResult result;
    result.message_ = message;
    AddJobSaveOutcome outcome{
        request.operationId_,
        request.cancellation_,
        request.draft_.jobTitle_,
        std::move(result)};
    QMetaObject::invokeMethod(
        this,
        [this, outcome = std::move(outcome)]() mutable {
            emit saveCompleted(outcome);
        },
        Qt::QueuedConnection);
}

void JobSaveWorker::queueUnavailableOutcome(
    const UpdateJobRequest& request,
    const QString& message)
{
    UpdateJobResult result;
    result.message_ = message;
    UpdateJobSaveOutcome outcome{
        request.operationId_,
        request.cancellation_,
        request.applicationId_,
        request.draft_.jobTitle_,
        std::move(result)};
    QMetaObject::invokeMethod(
        this,
        [this, outcome = std::move(outcome)]() mutable {
            emit updateCompleted(outcome);
        },
        Qt::QueuedConnection);
}

bool JobSaveWorker::isActiveOutcome(
    quint64 operationId,
    const std::shared_ptr<CancellationState>& cancellation) const
{
    return busy_
        && activeOperationId_ == operationId
        && activeCancellation_ == cancellation;
}

void JobSaveWorker::clearActiveRequest()
{
    busy_ = false;
    activeOperationId_ = 0;
    activeCancellation_.reset();
}

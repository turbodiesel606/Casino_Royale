#include "DataRemovalWorker.hpp"

#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "jobs/JobRepository.hpp"
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
    } catch (const std::exception& error) {
        const auto message = QString::fromUtf8(error.what());
        if (!message.isEmpty()) {
            return message;
        }
    } catch (...) {
    }
    return QStringLiteral("An unexpected deletion worker error occurred.");
}

}

class DataRemovalWorker::Executor final : public QObject
{
public:
    explicit Executor(QString dataDirectory)
        : dataDirectory_{std::move(dataDirectory)}
    {
    }

    void process(DataRemovalRequest request, DataRemovalWorker* facade)
    {
        Q_ASSERT(QThread::currentThread() == thread());
        DataRemovalBatchOutcome outcome;
        outcome.operationId_ = request.operationId_;
        outcome.kind_ = request.kind_;
        outcome.cancellation_ = request.cancellation_;
        try {
            auto& context = ensureContext();
            outcome.result_ = context.service_.process(
                request.kind_,
                request.items_,
                request.cancellation_);
        } catch (...) {
            const auto message = exceptionMessage(std::current_exception());
            for (const auto& item : request.items_) {
                outcome.result_.items_.append(
                    {item.id_, item.label_, DataRemovalItemStatus::Failed, {}, message});
            }
        }
        postOutcome(facade, std::move(outcome));
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
            , jobRepository_{database_.connection()}
            , cvRepository_{database_.connection()}
            , managedFileStore_{storagePaths_}
            , service_{
                  database_.connection(),
                  jobRepository_,
                  cvRepository_,
                  managedFileStore_}
        {
        }

        StoragePaths storagePaths_;
        SqliteDatabase database_;
        JobRepository jobRepository_;
        CvRepository cvRepository_;
        CvManagedFileStore managedFileStore_;
        DataRemovalService service_;
    };

    PipelineContext& ensureContext()
    {
        if (context_ == nullptr) {
            context_ = std::make_unique<PipelineContext>(dataDirectory_);
        }
        return *context_;
    }

    static void postOutcome(DataRemovalWorker* facade, DataRemovalBatchOutcome outcome)
    {
        QMetaObject::invokeMethod(
            facade,
            [facade, outcome = std::move(outcome)]() mutable {
                facade->deliverOutcome(std::move(outcome));
            },
            Qt::QueuedConnection);
    }

    QString dataDirectory_;
    std::unique_ptr<PipelineContext> context_;
};

DataRemovalWorker::DataRemovalWorker(QString dataDirectory, QObject* parent)
    : QObject{parent}
    , dataDirectory_{std::move(dataDirectory)}
{
    workerThread_.setObjectName(QStringLiteral("DataRemovalWorkerThread"));
    qRegisterMetaType<DataRemovalBatchOutcome>();
}

DataRemovalWorker::~DataRemovalWorker()
{
    shutdown();
}

void DataRemovalWorker::submit(DataRemovalRequest request)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (shuttingDown_ || busy_) {
        queueUnavailableOutcome(
            request,
            shuttingDown_
                ? QStringLiteral("The deletion worker is shutting down.")
                : QStringLiteral("Another deletion batch is already active."));
        return;
    }

    if (executor_ == nullptr) {
        executor_ = new Executor{dataDirectory_};
        executor_->moveToThread(&workerThread_);
        connect(&workerThread_, &QThread::finished, executor_, &QObject::deleteLater);
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

void DataRemovalWorker::shutdown()
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
            [executor = executor_]() { executor->destroyContext(); },
            Qt::BlockingQueuedConnection);
        workerThread_.quit();
        workerThread_.wait();
        executor_ = nullptr;
    }
    clearActiveRequest();
}

void DataRemovalWorker::deliverOutcome(DataRemovalBatchOutcome outcome)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (shuttingDown_ || !isActiveOutcome(outcome.operationId_, outcome.cancellation_)) {
        return;
    }
    clearActiveRequest();
    emit removalCompleted(outcome);
}

void DataRemovalWorker::queueUnavailableOutcome(
    const DataRemovalRequest& request,
    const QString& message)
{
    DataRemovalBatchOutcome outcome;
    outcome.operationId_ = request.operationId_;
    outcome.kind_ = request.kind_;
    outcome.cancellation_ = request.cancellation_;
    for (const auto& item : request.items_) {
        outcome.result_.items_.append(
            {item.id_, item.label_, DataRemovalItemStatus::Failed, {}, message});
    }
    QMetaObject::invokeMethod(
        this,
        [this, outcome = std::move(outcome)]() mutable {
            emit removalCompleted(outcome);
        },
        Qt::QueuedConnection);
}

bool DataRemovalWorker::isActiveOutcome(
    quint64 operationId,
    const std::shared_ptr<CancellationState>& cancellation) const
{
    return busy_
        && activeOperationId_ == operationId
        && activeCancellation_ == cancellation;
}

void DataRemovalWorker::clearActiveRequest()
{
    busy_ = false;
    activeOperationId_ = 0;
    activeCancellation_.reset();
}

#include "DataRemovalWorker.hpp"

#include "common/ExceptionUtils.hpp"
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

DataRemovalBatchOutcome failedBatchOutcome(
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
    return outcome;
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
            outcome = failedBatchOutcome(
                request,
                common::exceptionMessage(
                    std::current_exception(),
                    QStringLiteral("An unexpected deletion worker error occurred.")));
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
    , runtime_{
          QStringLiteral("DataRemovalWorkerThread"),
          {
              [dataDirectory = std::move(dataDirectory)]() -> QObject* {
                  return new Executor{dataDirectory};
              },
              [](QObject& executor) {
                  static_cast<Executor&>(executor).destroyContext();
              },
          }}
{
    qRegisterMetaType<DataRemovalBatchOutcome>();
}

DataRemovalWorker::~DataRemovalWorker()
{
    shutdown();
}

void DataRemovalWorker::submit(DataRemovalRequest request)
{
    Q_ASSERT(QThread::currentThread() == thread());
    SingleActiveWorkerAdmission admission;
    try {
        admission = runtime_.tryBeginOperation(
            request.operationId_,
            request.cancellation_);
    }
    catch (...) {
        queueUnavailableOutcome(
            request,
            common::exceptionMessage(
                std::current_exception(),
                QStringLiteral("An unexpected deletion worker error occurred.")));
        return;
    }
    if (admission != SingleActiveWorkerAdmission::Accepted) {
        queueUnavailableOutcome(
            request,
            admission == SingleActiveWorkerAdmission::ShuttingDown
                ? QStringLiteral("The deletion worker is shutting down.")
                : QStringLiteral("Another deletion batch is already active."));
        return;
    }

    auto* const executor = static_cast<Executor*>(&runtime_.executor());
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
    runtime_.shutdown();
}

void DataRemovalWorker::deliverOutcome(DataRemovalBatchOutcome outcome)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (!runtime_.isActiveOutcome(outcome.operationId_, outcome.cancellation_)) {
        return;
    }
    runtime_.completeOperation();
    emit removalCompleted(outcome);
}

void DataRemovalWorker::queueUnavailableOutcome(
    const DataRemovalRequest& request,
    const QString& message)
{
    auto outcome = failedBatchOutcome(request, message);
    QMetaObject::invokeMethod(
        this,
        [this, outcome = std::move(outcome)]() mutable {
            emit removalCompleted(outcome);
        },
        Qt::QueuedConnection);
}

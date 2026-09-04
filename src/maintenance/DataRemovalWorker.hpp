#ifndef JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALWORKER_HPP
#define JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALWORKER_HPP

#include "DataRemovalService.hpp"
#include "common/CancellationState.hpp"
#include "common/SingleActiveWorkerRuntime.hpp"

#include <QMetaType>
#include <QObject>

#include <memory>

struct DataRemovalRequest final
{
    quint64 operationId_ = 0;
    DataRemovalKind kind_ = DataRemovalKind::DeleteJobs;
    QVector<DataRemovalItemRequest> items_;
    std::shared_ptr<CancellationState> cancellation_;
};

struct DataRemovalBatchOutcome final
{
    quint64 operationId_ = 0;
    DataRemovalKind kind_ = DataRemovalKind::DeleteJobs;
    std::shared_ptr<CancellationState> cancellation_;
    DataRemovalBatchResult result_;
};

Q_DECLARE_METATYPE(DataRemovalBatchOutcome)

// GUI-thread facade for one reusable best-effort data-removal worker.
class DataRemovalWorker final : public QObject
{
    Q_OBJECT

public:
    explicit DataRemovalWorker(QString dataDirectory, QObject* parent = nullptr);
    ~DataRemovalWorker() override;

    void submit(DataRemovalRequest request);
    void shutdown();

signals:
    void removalCompleted(const DataRemovalBatchOutcome& outcome);

private:
    class Executor;

    void deliverOutcome(DataRemovalBatchOutcome outcome);
    void queueUnavailableOutcome(const DataRemovalRequest& request, const QString& message);

    SingleActiveWorkerRuntime runtime_;
};

#endif // JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALWORKER_HPP

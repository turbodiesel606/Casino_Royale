#ifndef JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALWORKER_HPP
#define JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALWORKER_HPP

#include "DataRemovalService.hpp"
#include "common/CancellationState.hpp"

#include <QMetaType>
#include <QObject>
#include <QThread>

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
    bool isActiveOutcome(
        quint64 operationId,
        const std::shared_ptr<CancellationState>& cancellation) const;
    void clearActiveRequest();

    QString dataDirectory_;
    QThread workerThread_;
    Executor* executor_ = nullptr;
    std::shared_ptr<CancellationState> activeCancellation_;
    quint64 activeOperationId_ = 0;
    bool busy_ = false;
    bool shuttingDown_ = false;
};

#endif // JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALWORKER_HPP

#ifndef JOBTRACKER_SRC_JOBS_ADDJOBWORKER_HPP
#define JOBTRACKER_SRC_JOBS_ADDJOBWORKER_HPP

#include "AddJobService.hpp"
#include "common/CancellationState.hpp"

#include <QMetaType>
#include <QObject>
#include <QString>
#include <QThread>
#include <QUrl>

#include <memory>

// Carries one controller-validated Add Job request into the worker thread.
struct AddJobRequest final
{
    quint64 operationId_ = 0;
    NormalizedJobApplicationDraft draft_;
    QUrl selectedCvUrl_;
    std::shared_ptr<CancellationState> cancellation_;
};

// Returns the value-only durable result after worker-side CV and SQL work.
struct AddJobSaveOutcome final
{
    quint64 operationId_ = 0;
    std::shared_ptr<CancellationState> cancellation_;
    QString jobTitle_;
    AddJobResult result_;
};

Q_DECLARE_METATYPE(AddJobSaveOutcome)

// GUI-thread facade for one reusable Add Job worker thread and its private
// persistence context. The facade accepts at most one active request.
class AddJobWorker final : public QObject
{
    Q_OBJECT

public:
    explicit AddJobWorker(QString dataDirectory, QObject* parent = nullptr);
    ~AddJobWorker() override;

    void submit(AddJobRequest request);
    void shutdown();
    bool isRunning() const;

signals:
    void saveCompleted(const AddJobSaveOutcome& outcome);

private:
    class Executor;

    void deliverSaveOutcome(AddJobSaveOutcome outcome);
    void queueUnavailableOutcome(const AddJobRequest& request, const QString& message);
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

#endif // JOBTRACKER_SRC_JOBS_ADDJOBWORKER_HPP

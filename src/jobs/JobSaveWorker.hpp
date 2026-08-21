#ifndef JOBTRACKER_SRC_JOBS_JOBSAVEWORKER_HPP
#define JOBTRACKER_SRC_JOBS_JOBSAVEWORKER_HPP

#include "AddJobService.hpp"
#include "UpdateJobService.hpp"
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

struct UpdateJobRequest final
{
    quint64 operationId_ = 0;
    QString applicationId_;
    NormalizedJobApplicationDraft draft_;
    QUrl replacementCvUrl_;
    std::shared_ptr<CancellationState> cancellation_;
};

struct UpdateJobSaveOutcome final
{
    quint64 operationId_ = 0;
    std::shared_ptr<CancellationState> cancellation_;
    QString applicationId_;
    QString jobTitle_;
    UpdateJobResult result_;
};

Q_DECLARE_METATYPE(UpdateJobSaveOutcome)

// GUI-thread facade for one reusable job-save worker thread and its private
// persistence context. The controller submits at most one active request.
class JobSaveWorker final : public QObject
{
    Q_OBJECT

public:
    explicit JobSaveWorker(QString dataDirectory, QObject* parent = nullptr);
    ~JobSaveWorker() override;

    void submit(AddJobRequest request);
    void submit(UpdateJobRequest request);
    void shutdown();
    bool isRunning() const;

signals:
    void saveCompleted(const AddJobSaveOutcome& outcome);
    void updateCompleted(const UpdateJobSaveOutcome& outcome);

private:
    class Executor;

    void deliverSaveOutcome(AddJobSaveOutcome outcome);
    void deliverUpdateOutcome(UpdateJobSaveOutcome outcome);
    void queueUnavailableOutcome(const AddJobRequest& request, const QString& message);
    void queueUnavailableOutcome(const UpdateJobRequest& request, const QString& message);
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

#endif // JOBTRACKER_SRC_JOBS_JOBSAVEWORKER_HPP

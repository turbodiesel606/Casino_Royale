#ifndef JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALEXECUTOR_HPP
#define JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALEXECUTOR_HPP

#include "DataRemovalService.hpp"
#include "DataRemovalTypes.hpp"
#include "common/ExceptionUtils.hpp"
#include "logging/ApplicationLog.hpp"

#include <QObject>
#include <QString>
#include <QThread>

#include <exception>
#include <memory>
#include <variant>

// Owns the removal persistence graph on its worker thread and returns value outcomes.
class DataRemovalExecutor final : public QObject
{
public:
    using Outcome = std::variant<
        DeleteJobsOutcome,
        RemoveActiveCvsOutcome,
        RestoreArchivedCvsOutcome,
        DeleteArchivedCvsOutcome>;

    explicit DataRemovalExecutor(QString dataDirectory);
    ~DataRemovalExecutor() override;

    template<DataRemovalKind Kind>
        requires SupportedDataRemovalKind<Kind>
    DataRemovalBatchOutcome<Kind> process(DataRemovalRequest<Kind> request)
    {
        /* 
        if you want to preserve this assertion in release version, enable "Release with QT_FORCE_ASSERTS".
        this assertion happens only in debug build.
        Qt disables it when QT_NO_DEBUG is defined and QT_FORCE_ASSERTS is not defined.
        */

        constexpr auto operation = removalLogOperation(Kind);
        if (QThread::currentThread() != thread()) {
            ApplicationLog::record({LogEventType::BackgroundWorkInvariantViolated,
                {operation, LogFact::RemovalWrongThread, LogFact::RemovalExecution}, request.operationId_});
        }
        Q_ASSERT(QThread::currentThread() == thread());

        // Preserve request id so the GUI can recognize which active batch this result belongs to

        DataRemovalBatchOutcome<Kind> outcome;
        outcome.operationId_ = request.operationId_;
        outcome.cancellation_ = request.cancellation_;

        try {
            outcome.result_ = ensureService(operation, request.items_.size()).template process<Kind>(
                request.items_,
                request.cancellation_, request.operationId_);
        } catch (...) {
            outcome = maintenance::detail::failedBatchOutcome(
                request,
                common::exceptionMessage(
                    std::current_exception(),
                    QStringLiteral("An unexpected deletion worker error occurred.")));
        }
        return outcome;
    }

    void destroyContext();

private:
    struct PipelineContext;

    DataRemovalService& ensureService(LogFact operation, qint64 count);

    QString dataDirectory_;
    std::unique_ptr<PipelineContext> context_;
};

#endif // JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALEXECUTOR_HPP

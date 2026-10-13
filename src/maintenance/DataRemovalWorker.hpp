#ifndef JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALWORKER_HPP
#define JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALWORKER_HPP

#include "DataRemovalExecutor.hpp"
#include "DataRemovalTypes.hpp"
#include "common/SingleActiveWorkerFacade.hpp"

#include <QLatin1StringView>
#include <QObject>
#include <QString>

template<DataRemovalKind Kind>
    requires SupportedDataRemovalKind<Kind>
struct SingleActiveWorkerRequestTraits<DataRemovalRequest<Kind>>
{
    static constexpr auto errorMessage =
        QLatin1StringView{"An unexpected deletion worker error occurred."};
    static constexpr auto shuttingDownMessage =
        QLatin1StringView{"The deletion worker is shutting down."};
    static constexpr auto busyMessage =
        QLatin1StringView{"Another deletion batch is already active."};

    static DataRemovalBatchOutcome<Kind> unavailableOutcome(
        const DataRemovalRequest<Kind>& request,
        const QString& message)
    {
        return maintenance::detail::failedBatchOutcome(request, message);
    }
};

// GUI-thread facade for one reusable best-effort data-removal worker.
class DataRemovalWorker final
    : public SingleActiveWorkerFacade<DataRemovalExecutor>
{
    Q_OBJECT

    using Base = SingleActiveWorkerFacade<DataRemovalExecutor>;

public:
    explicit DataRemovalWorker(QString dataDirectory, QObject* parent = nullptr);

signals:
    void jobsDeletionCompleted(const DeleteJobsOutcome& outcome);
    void activeCvsRemovalCompleted(const RemoveActiveCvsOutcome& outcome);
    void archivedCvsRestorationCompleted(const RestoreArchivedCvsOutcome& outcome);
    void archivedCvsDeletionCompleted(const DeleteArchivedCvsOutcome& outcome);
};

#endif // JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALWORKER_HPP

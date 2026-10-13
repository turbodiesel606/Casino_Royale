#include "DataRemovalWorker.hpp"

#include <QMetaType>

#include <type_traits>
#include <utility>
#include <variant>

DataRemovalWorker::DataRemovalWorker(QString dataDirectory, QObject* parent)
    : Base{
          QStringLiteral("DataRemovalWorkerThread"),
          std::move(dataDirectory),
          parent}
{
    qRegisterMetaType<DeleteJobsOutcome>();
    qRegisterMetaType<RemoveActiveCvsOutcome>();
    qRegisterMetaType<RestoreArchivedCvsOutcome>();
    qRegisterMetaType<DeleteArchivedCvsOutcome>();

    setOutcomeHandler(
        [this](Base::Outcome outcome) {
            std::visit(
                [this](const auto& value) {
                    using OutcomeType = std::remove_cvref_t<decltype(value)>;
                    if constexpr (OutcomeType::kind_ == DataRemovalKind::DeleteJobs) {
                        emit jobsDeletionCompleted(value);
                    } else if constexpr (OutcomeType::kind_ == DataRemovalKind::RemoveActiveCvs) {
                        emit activeCvsRemovalCompleted(value);
                    } else if constexpr (OutcomeType::kind_ == DataRemovalKind::RestoreArchivedCvs) {
                        emit archivedCvsRestorationCompleted(value);
                    } else {
                        emit archivedCvsDeletionCompleted(value);
                    }
                },
                outcome);
        });
}

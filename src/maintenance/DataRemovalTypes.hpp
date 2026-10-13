#ifndef JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALTYPES_HPP
#define JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALTYPES_HPP

#include "cvs/CvDocument.hpp"

#include <QMetaType>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include <memory>

class CancellationState;

enum class DataRemovalKind
{
    DeleteJobs,
    RemoveActiveCvs,
    RestoreArchivedCvs,
    DeleteArchivedCvs
};

// Constrain the public protocol and typed service/executor entry points.
template<DataRemovalKind Kind>
concept SupportedDataRemovalKind =
    Kind == DataRemovalKind::DeleteJobs
    || Kind == DataRemovalKind::RemoveActiveCvs
    || Kind == DataRemovalKind::RestoreArchivedCvs
    || Kind == DataRemovalKind::DeleteArchivedCvs;

enum class DataRemovalItemStatus
{
    Deleted,
    Archived,
    Restored,
    SkippedLinked,
    Failed
};

struct DataRemovalItemRequest final
{
    QString id_;
    QString label_;
};

struct DataRemovalItemOutcome final
{
    QString id_;
    QString label_;
    DataRemovalItemStatus status_ = DataRemovalItemStatus::Failed;
    CvDocument document_;
    QString message_;
};

struct DataRemovalBatchResult final
{
    QVector<DataRemovalItemOutcome> items_;
    bool cancelled_ = false;
};

// The operation kind travels in the type; only request data crosses threads.
template<DataRemovalKind Kind>
    requires SupportedDataRemovalKind<Kind>
struct DataRemovalRequest final
{
    static constexpr DataRemovalKind kind_ = Kind;

    quint64 operationId_ = 0;
    QVector<DataRemovalItemRequest> items_;
    std::shared_ptr<CancellationState> cancellation_;
};

// Runtime correlation remains necessary for successive requests of the same kind.
template<DataRemovalKind Kind>
    requires SupportedDataRemovalKind<Kind>
struct DataRemovalBatchOutcome final
{
    static constexpr DataRemovalKind kind_ = Kind;

    quint64 operationId_ = 0;
    std::shared_ptr<CancellationState> cancellation_;
    DataRemovalBatchResult result_;
};

using DeleteJobsOutcome = DataRemovalBatchOutcome<DataRemovalKind::DeleteJobs>;
using RemoveActiveCvsOutcome = DataRemovalBatchOutcome<DataRemovalKind::RemoveActiveCvs>;
using RestoreArchivedCvsOutcome = DataRemovalBatchOutcome<DataRemovalKind::RestoreArchivedCvs>;
using DeleteArchivedCvsOutcome = DataRemovalBatchOutcome<DataRemovalKind::DeleteArchivedCvs>;

namespace maintenance::detail {

// Preserve request correlation and item order for infrastructure failures.
template<DataRemovalKind Kind>
    requires SupportedDataRemovalKind<Kind>
DataRemovalBatchOutcome<Kind> failedBatchOutcome(
    const DataRemovalRequest<Kind>& request,
    const QString& message)
{
    DataRemovalBatchOutcome<Kind> outcome;
    outcome.operationId_ = request.operationId_;
    outcome.cancellation_ = request.cancellation_;
    for (const auto& item : request.items_) {
        outcome.result_.items_.append(
            {item.id_, item.label_, DataRemovalItemStatus::Failed, {}, message});
    }
    return outcome;
}

} // namespace maintenance::detail

Q_DECLARE_METATYPE(DeleteJobsOutcome)
Q_DECLARE_METATYPE(RemoveActiveCvsOutcome)
Q_DECLARE_METATYPE(RestoreArchivedCvsOutcome)
Q_DECLARE_METATYPE(DeleteArchivedCvsOutcome)

#endif // JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALTYPES_HPP

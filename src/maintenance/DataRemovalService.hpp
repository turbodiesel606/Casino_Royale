#ifndef JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALSERVICE_HPP
#define JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALSERVICE_HPP

#include "cvs/CvDocument.hpp"

#include <QString>
#include <QVector>

#include <memory>

class CancellationState;
class CvManagedFileStore;
class CvRepository;
class JobRepository;
class QSqlDatabase;
class SqlTransaction;

enum class DataRemovalKind
{
    DeleteJobs,
    RemoveActiveCvs,
    RestoreArchivedCvs,
    DeleteArchivedCvs
};

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

// Coordinates best-effort per-item job/CV removal transactions and managed
// file compensation. The service and all dependencies stay on one SQL thread.
class DataRemovalService final
{
public:
    DataRemovalService(
        QSqlDatabase& database,
        JobRepository& jobRepository,
        CvRepository& cvRepository,
        CvManagedFileStore& managedFileStore);

    DataRemovalBatchResult process(
        DataRemovalKind kind,
        const QVector<DataRemovalItemRequest>& items,
        const std::shared_ptr<CancellationState>& cancellation) const;

private:
    DataRemovalItemOutcome deleteJob(const DataRemovalItemRequest& item) const;
    DataRemovalItemOutcome removeActiveCv(const DataRemovalItemRequest& item) const;
    DataRemovalItemOutcome restoreArchivedCv(const DataRemovalItemRequest& item) const;
    DataRemovalItemOutcome deleteArchivedCv(const DataRemovalItemRequest& item) const;
    DataRemovalItemOutcome deleteUnlinkedCv(
        const DataRemovalItemRequest& item,
        const CvDocument& document,
        bool protectedLinkedCv,
        SqlTransaction& transaction) const;

    QSqlDatabase& database_;
    JobRepository& jobRepository_;
    CvRepository& cvRepository_;
    CvManagedFileStore& managedFileStore_;
};

#endif // JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALSERVICE_HPP

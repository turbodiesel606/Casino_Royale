#include "DataRemovalExecutor.hpp"

#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "jobs/JobRepository.hpp"
#include "storage/SqliteDatabase.hpp"
#include "storage/StoragePaths.hpp"

#include <utility>

struct DataRemovalExecutor::PipelineContext final
{
    PipelineContext(const QString& dataDirectory, LogFact operation, qint64 count)
        : storagePaths_{dataDirectory, operation, count}
        , database_{storagePaths_.databasePath(), operation, count}
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

DataRemovalExecutor::DataRemovalExecutor(QString dataDirectory)
    : dataDirectory_{std::move(dataDirectory)}
{
}

DataRemovalExecutor::~DataRemovalExecutor() = default;

void DataRemovalExecutor::destroyContext()
{
    Q_ASSERT(QThread::currentThread() == thread());
    context_.reset();
}

DataRemovalService& DataRemovalExecutor::ensureService(LogFact operation, qint64 count)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (context_ == nullptr)
        context_ = std::make_unique<PipelineContext>(dataDirectory_, operation, count);
    
    return context_->service_;
}

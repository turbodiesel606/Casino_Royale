#ifndef JOBTRACKER_SRC_CVS_CVMANAGEDFILESTORE_HPP
#define JOBTRACKER_SRC_CVS_CVMANAGEDFILESTORE_HPP

#include "CvDocument.hpp"
#include "CvManagedPathResolver.hpp"
#include "common/CancellationState.hpp"
#include "logging/LogEvent.hpp"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

#include <memory>

class StoragePaths;

// Owns the source snapshot and removes any unfinished stage on destruction.

struct CvManagedFilePreparation final
{
    CvManagedFilePreparation() = default;
    CvManagedFilePreparation(const CvManagedFilePreparation&) = delete;
    CvManagedFilePreparation& operator=(const CvManagedFilePreparation&) = delete;
    ~CvManagedFilePreparation();

    QByteArray bytes_;
    QString originalFileName_;
    QString storedFileName_;
    QString relativePath_;
    QString stagedFilePath_;
    QString finalFilePath_; // Set only after a successful final rename.
    LogFact stagedCleanup_ = LogFact::NotAttempted;
    LogFact cleanupOperation_ = LogFact::CvImport;
    quint64 cleanupOperationId_ = 0;
    QString sha256_;
    qint64 sizeBytes_ = 0;
};

// Reports the outcome of preparing a managed CV file.

struct CvManagedFilePreparationResult final
{
    std::shared_ptr<CvManagedFilePreparation> preparation_;
    QString message_;
    bool cancelled_ = false;
    LogFact stage_ = LogFact::PersistSourceInspection;
    LogFact cause_ = LogFact::CauseUnavailable;

    bool succeeded() const;
};

struct CvManagedFileRecoveryReport final
{
    int removedStagedFileCount_ = 0;
    int removedDeletionFileCount_ = 0;
    int restoredDeletionFileCount_ = 0;
    QStringList quarantinedFileNames_;
};

struct CvManagedFileRemovalPreparation final
{
    CvManagedFileRemovalPreparation() = default;
    CvManagedFileRemovalPreparation(const CvManagedFileRemovalPreparation&) = delete;
    CvManagedFileRemovalPreparation& operator=(const CvManagedFileRemovalPreparation&) = delete;
    ~CvManagedFileRemovalPreparation();
    LogFact restoreRetainedFile() noexcept;

    QString originalFilePath_;
    QString tombstoneFilePath_;
    bool databaseCommitted_ = false;
    quint64 operationId_ = 0;
private:
    bool restorationChecked_ = false;
};

struct CvManagedFileRemovalPreparationResult final
{
    std::shared_ptr<CvManagedFileRemovalPreparation> preparation_;
    QString message_;
    bool fileWasMissing_ = false;
    LogFact failure_ = LogFact::None;

    bool succeeded() const;
};

struct CvManagedFileStageResult final
{
    QString message_;
    bool cancelled_ = false;
    LogFact stage_ = LogFact::PersistCreateStage;
    LogFact cause_ = LogFact::FileError;

    bool succeeded() const;
};

// Manages CV files on disk and directly performs filesystem operations.

class CvManagedFileStore final
{
public:
    explicit CvManagedFileStore(const StoragePaths& paths);

    CvManagedFilePreparationResult prepare(
        const QUrl& sourceUrl,
        const std::shared_ptr<CancellationState>& cancellation) const;
    CvManagedFileStageResult stageAndFinalize(
        CvManagedFilePreparation& preparation,
        const std::shared_ptr<CancellationState>& cancellation,
        LogFact operation = LogFact::CvImport, quint64 operationId = 0) const;
    bool removeCompletedFile(const QString& completedFilePath, LogFact* cleanup = nullptr) const;
    CvManagedFileRemovalPreparationResult prepareRemoval(const CvDocument& document) const;
    bool finalizeRemoval(CvManagedFileRemovalPreparation& preparation) const;
    CvManagedFileRecoveryReport reconcile(const QVector<CvDocument>& documents) const;

    QString quarantineDirectory() const;

private:
    const StoragePaths& paths_;
    CvManagedPathResolver pathResolver_;
};

#endif // JOBTRACKER_SRC_CVS_CVMANAGEDFILESTORE_HPP

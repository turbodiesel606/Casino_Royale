#ifndef JOBTRACKER_SRC_CVS_CVMANAGEDFILESTORE_HPP
#define JOBTRACKER_SRC_CVS_CVMANAGEDFILESTORE_HPP

#include "CvDocument.hpp"
#include "common/CancellationState.hpp"

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

#include <memory>

class StoragePaths;

// Holds a staged CV file and removes unfinished data on destruction.

struct CvManagedFilePreparation final
{
    CvManagedFilePreparation() = default;
    CvManagedFilePreparation(const CvManagedFilePreparation&) = delete;
    CvManagedFilePreparation& operator=(const CvManagedFilePreparation&) = delete;
    ~CvManagedFilePreparation();

    QString originalFileName_;
    QString storedFileName_;
    QString relativePath_;
    QString stagedFilePath_;
    QString finalFilePath_;
    QString sha256_;
    qint64 sizeBytes_ = 0;
};

// Reports the outcome of preparing a managed CV file.

struct CvManagedFilePreparationResult final
{
    std::shared_ptr<CvManagedFilePreparation> preparation_;
    QString message_;
    bool cancelled_ = false;

    bool succeeded() const;
};

struct CvManagedFileRecoveryReport final
{
    int removedStagedFileCount_ = 0;
    QStringList quarantinedFileNames_;
};

// Manages CV files on disk and directly performs filesystem operations.

class CvManagedFileStore final
{
public:
    explicit CvManagedFileStore(const StoragePaths& paths);

    CvManagedFilePreparationResult prepare(
        const QUrl& sourceUrl,
        const std::shared_ptr<CancellationState>& cancellation) const;
    QString finalize(CvManagedFilePreparation& preparation) const;
    bool removeCompletedFile(const QString& completedFilePath) const;
    CvManagedFileRecoveryReport reconcile(const QVector<CvDocument>& documents) const;

    QString quarantineDirectory() const;

private:
    const StoragePaths& paths_;
};

#endif // JOBTRACKER_SRC_CVS_CVMANAGEDFILESTORE_HPP

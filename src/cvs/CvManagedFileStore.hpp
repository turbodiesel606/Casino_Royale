#ifndef JOBTRACKER_SRC_CVS_CVMANAGEDFILESTORE_HPP
#define JOBTRACKER_SRC_CVS_CVMANAGEDFILESTORE_HPP

#include "CvDocument.hpp"

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

#include <atomic>
#include <memory>

class StoragePaths;

// Owns managed CV filesystem preparation and crash-recovery behavior.
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

class CvManagedFileStore final
{
public:
    explicit CvManagedFileStore(const StoragePaths& paths);

    CvManagedFilePreparationResult prepare(
        const QUrl& sourceUrl,
        const std::shared_ptr<std::atomic_bool>& cancellation) const;
    QString finalize(CvManagedFilePreparation& preparation) const;
    bool removeCompletedFile(const QString& completedFilePath) const;
    CvManagedFileRecoveryReport reconcile(const QVector<CvDocument>& documents) const;

    QString quarantineDirectory() const;

private:
    const StoragePaths& paths_;
};

#endif // JOBTRACKER_SRC_CVS_CVMANAGEDFILESTORE_HPP

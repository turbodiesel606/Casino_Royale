#ifndef JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP
#define JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP

#include "CvDocument.hpp"
#include "CvManagedFileStore.hpp"

#include <QUrl>

#include <atomic>
#include <memory>

class CvRepository;

struct CvImportResult final
{
    CvDocument document_;
    QString completedFilePath_;
    bool wasInserted_ = false;
};

// Coordinates worker-safe CV file preparation with database-thread CV persistence.
// Reuses an existing CV by identity or finalizes a new managed file and inserts its metadata into SQLite.

class CvImportService final
{
public:
    CvImportService(const CvManagedFileStore& managedFileStore, CvRepository& repository);

    CvManagedFilePreparationResult prepareDocument(
        const QUrl& sourceUrl,
        const std::shared_ptr<std::atomic_bool>& cancellation) const;
    CvImportResult importPreparedDocument(
        const std::shared_ptr<CvManagedFilePreparation>& preparation) const;
    bool removeCompletedFile(const QString& completedFilePath) const;

private:
    const CvManagedFileStore& managedFileStore_;
    CvRepository& repository_;
};

#endif // JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP

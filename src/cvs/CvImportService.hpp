#ifndef JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP
#define JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP

#include "CvDocument.hpp"
#include "CvManagedFileStore.hpp"
#include "common/CancellationState.hpp"

#include <QUrl>

#include <memory>

class CvRepository;

enum class CvImportDisposition
{
    Inserted,
    ExistingActive,
    RestoredArchived
};

QString cvImportDispositionName(CvImportDisposition disposition);
QString cvImportSuccessMessage(CvImportDisposition disposition);

struct CvImportResult final
{
    CvDocument document_;
    QString completedFilePath_;
    CvImportDisposition disposition_ = CvImportDisposition::ExistingActive;
    bool success_ = false;
    bool cancelled_ = false;
    QString message_;
};

// Coordinates CV file preparation with persistence. Reuses an existing CV by
// identity or finalizes a new managed file and inserts its metadata into SQLite.

class CvImportService final
{
public:
    CvImportService(const CvManagedFileStore& managedFileStore, CvRepository& repository);

    CvManagedFilePreparationResult prepareDocument(
        const QUrl& sourceUrl,
        const std::shared_ptr<CancellationState>& cancellation) const;
    // Caller holds the shared CV lease and transaction. SQL failures propagate;
    // after rollback, finalFilePath_ identifies any file requiring compensation.
    CvImportResult importPreparedDocument(
        const std::shared_ptr<CvManagedFilePreparation>& preparation,
        const std::shared_ptr<CancellationState>& cancellation) const;
    bool removeCompletedFile(const QString& completedFilePath) const;

private:
    const CvManagedFileStore& managedFileStore_;
    CvRepository& repository_;
};

#endif // JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP

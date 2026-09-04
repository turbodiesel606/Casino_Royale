#ifndef JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP
#define JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP

#include "CvDocument.hpp"
#include "CvManagedFileStore.hpp"
#include "common/CancellationState.hpp"

#include <QUrl>

#include <memory>

class CvRepository;

enum class CvArchivedDuplicatePolicy
{
    PreserveArchived,
    RestoreArchived
};

enum class CvImportDisposition
{
    Inserted,
    ExistingActive,
    RestoredArchived,
    ReusedArchived
};

QString cvImportDispositionName(CvImportDisposition disposition);
QString cvImportSuccessMessage(CvImportDisposition disposition);

struct CvImportResult final
{
    CvDocument document_;
    QString completedFilePath_;
    CvImportDisposition disposition_ = CvImportDisposition::ExistingActive;
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
    CvImportResult importPreparedDocument(
        const std::shared_ptr<CvManagedFilePreparation>& preparation,
        CvArchivedDuplicatePolicy archivedDuplicatePolicy = CvArchivedDuplicatePolicy::PreserveArchived) const;
    bool removeCompletedFile(const QString& completedFilePath) const;

private:
    const CvManagedFileStore& managedFileStore_;
    CvRepository& repository_;
};

#endif // JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP

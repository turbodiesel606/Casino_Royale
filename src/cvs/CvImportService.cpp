#include "CvImportService.hpp"

#include "CvRepository.hpp"
#include "common/TimeUtils.hpp"

#include <QFileInfo>
#include <QUuid>

#include <stdexcept>
#include <utility>

namespace {

CvDocument buildCvDocument(const CvManagedFilePreparation& preparation)
{
    CvDocument document;
    document.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
    document.originalFileName_ = preparation.originalFileName_;
    document.storedFileName_ = preparation.storedFileName_;
    document.relativePath_ = preparation.relativePath_;
    document.sha256_ = preparation.sha256_;
    document.sizeBytes_ = preparation.sizeBytes_;
    document.title_ = QFileInfo{preparation.originalFileName_}.completeBaseName();
    document.category_ = QStringLiteral("General");
    document.createdAt_ = common::currentUtcSecond();
    document.updatedAt_ = document.createdAt_;
    return document;
}

}

QString cvImportDispositionName(CvImportDisposition disposition)
{
    switch (disposition) {
    case CvImportDisposition::Inserted:
        return QStringLiteral("inserted");
    case CvImportDisposition::RestoredArchived:
        return QStringLiteral("restored-archived");
    case CvImportDisposition::ReusedArchived:
        return QStringLiteral("reused-archived");
    case CvImportDisposition::ExistingActive:
    default:
        return QStringLiteral("existing-active");
    }
}

QString cvImportSuccessMessage(CvImportDisposition disposition)
{
    switch (disposition) {
    case CvImportDisposition::Inserted:
        return QStringLiteral("CV added successfully.");
    case CvImportDisposition::RestoredArchived:
        return QStringLiteral("The archived CV was restored to the library.");
    case CvImportDisposition::ExistingActive:
    case CvImportDisposition::ReusedArchived:
    default:
        return QStringLiteral("A CV with the same filename and SHA-256 already exists.");
    }
}

CvImportService::CvImportService(
    const CvManagedFileStore& managedFileStore,
    CvRepository& repository)
    : managedFileStore_(managedFileStore)
    , repository_(repository)
{
}

CvManagedFilePreparationResult CvImportService::prepareDocument(
    const QUrl& sourceUrl,
    const std::shared_ptr<CancellationState>& cancellation) const
{
    return managedFileStore_.prepare(sourceUrl, cancellation);
}

CvImportResult CvImportService::importPreparedDocument(
    const std::shared_ptr<CvManagedFilePreparation>& preparation,
    CvArchivedDuplicatePolicy archivedDuplicatePolicy) const
{
    if (preparation == nullptr)
        throw std::runtime_error("The prepared CV file is unavailable.");
    
    // Consider replacing this with switch-case
    if (auto existing = repository_.findByIdentity(
            preparation->sha256_,
            preparation->originalFileName_)) {
        if (existing->archivedAt_.isValid()) {
            if (archivedDuplicatePolicy == CvArchivedDuplicatePolicy::RestoreArchived) {
                const auto updatedAt = repository_.updateArchived(existing->id_, false);
                if (!updatedAt) {
                    throw std::runtime_error("The archived CV could not be restored.");
                }
                existing->applyArchiveState(QDateTime{}, *updatedAt);
                return {*existing, {}, CvImportDisposition::RestoredArchived};
            }
            return {*existing, {}, CvImportDisposition::ReusedArchived};
        }
        return {*existing, {}, CvImportDisposition::ExistingActive};
    }

    auto document = buildCvDocument(*preparation);
    const auto completedFilePath = managedFileStore_.finalize(*preparation);
    try {
        repository_.insert(document);
    } catch (...) {
        managedFileStore_.removeCompletedFile(completedFilePath);
        throw;
    }
    return {std::move(document), completedFilePath, CvImportDisposition::Inserted};
}

bool CvImportService::removeCompletedFile(const QString& completedFilePath) const
{
    return managedFileStore_.removeCompletedFile(completedFilePath);
}

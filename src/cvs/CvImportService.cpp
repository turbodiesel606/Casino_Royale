#include "CvImportService.hpp"

#include "CvRepository.hpp"

#include <QDateTime>
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
    document.createdAt_ = QDateTime::fromString(
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate),
        Qt::ISODate).toUTC();
    document.updatedAt_ = document.createdAt_;
    return document;
}

}

QString cvImportDispositionName(CvImportDisposition disposition)
{
    switch (disposition) {
    case CvImportDisposition::Inserted:
        return QStringLiteral("inserted");
    case CvImportDisposition::ExistingActive:
        return QStringLiteral("existing-active");
    case CvImportDisposition::RestoredArchived:
        return QStringLiteral("restored-archived");
    case CvImportDisposition::ReusedArchived:
        return QStringLiteral("reused-archived");
    }
    return QStringLiteral("existing-active");
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
    if (preparation == nullptr) {
        throw std::runtime_error("The prepared CV file is unavailable.");
    }

    if (auto existing = repository_.findByIdentity(
            preparation->sha256_,
            preparation->originalFileName_)) {
        if (existing->archivedAt_.isValid()) {
            if (archivedDuplicatePolicy == CvArchivedDuplicatePolicy::RestoreArchived) {
                const auto updatedAt = repository_.updateArchived(existing->id_, false);
                if (!updatedAt) {
                    throw std::runtime_error("The archived CV could not be restored.");
                }
                existing->archivedAt_ = {};
                existing->updatedAt_ = *updatedAt;
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

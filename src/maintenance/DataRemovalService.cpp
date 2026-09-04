#include "DataRemovalService.hpp"

#include "common/CancellationState.hpp"
#include "common/ExceptionUtils.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "jobs/JobRepository.hpp"
#include "storage/SqlTransaction.hpp"

#include <QDateTime>
#include <QSqlDatabase>

#include <exception>
#include <stdexcept>
#include <utility>

namespace {

DataRemovalItemOutcome failureOutcome(
    const DataRemovalItemRequest& item,
    QString message)
{
    return {item.id_, item.label_, DataRemovalItemStatus::Failed, {}, std::move(message)};
}

}

DataRemovalService::DataRemovalService(
    QSqlDatabase& database,
    JobRepository& jobRepository,
    CvRepository& cvRepository,
    CvManagedFileStore& managedFileStore)
    : database_{database}
    , jobRepository_{jobRepository}
    , cvRepository_{cvRepository}
    , managedFileStore_{managedFileStore}
{
}

DataRemovalBatchResult DataRemovalService::process(
    DataRemovalKind kind,
    const QVector<DataRemovalItemRequest>& items,
    const std::shared_ptr<CancellationState>& cancellation) const
{
    DataRemovalBatchResult result;
    result.items_.reserve(items.size());
    for (const auto& item : items) {
        if (cancellation != nullptr && cancellation->isCancellationRequested()) {
            result.cancelled_ = true;
            break;
        }

        try {
            switch (kind) {
            case DataRemovalKind::DeleteJobs:
                result.items_.append(deleteJob(item));
                break;
            case DataRemovalKind::RemoveActiveCvs:
                result.items_.append(removeActiveCv(item));
                break;
            case DataRemovalKind::RestoreArchivedCvs:
                result.items_.append(restoreArchivedCv(item));
                break;
            case DataRemovalKind::DeleteArchivedCvs:
                result.items_.append(deleteArchivedCv(item));
                break;
            }
        } catch (...) {
            result.items_.append(failureOutcome(
                item,
                common::exceptionMessage(
                    std::current_exception(),
                    QStringLiteral("An unexpected deletion error occurred."))));
        }
    }
    return result;
}

DataRemovalItemOutcome DataRemovalService::deleteJob(const DataRemovalItemRequest& item) const
{
    SqlTransaction transaction{database_, QStringLiteral("delete job application")};
    if (!jobRepository_.remove(item.id_)) {
        return failureOutcome(item, QStringLiteral("The job application was not found."));
    }
    transaction.commit();
    return {item.id_, item.label_, DataRemovalItemStatus::Deleted, {}, {}};
}

DataRemovalItemOutcome DataRemovalService::removeActiveCv(const DataRemovalItemRequest& item) const
{
    SqlTransaction transaction{database_, QStringLiteral("remove CV from library")};
    auto document = cvRepository_.findById(item.id_);
    if (!document) {
        return failureOutcome(item, QStringLiteral("The CV was not found."));
    }
    if (document->archivedAt_.isValid()) {
        return failureOutcome(item, QStringLiteral("The CV is already archived."));
    }
    if (!document->linkedApplicationIds_.isEmpty()) {
        const auto updatedAt = cvRepository_.updateArchived(item.id_, true);
        if (!updatedAt) {
            return failureOutcome(item, QStringLiteral("The CV could not be archived."));
        }
        document->applyArchiveState(*updatedAt, *updatedAt);
        transaction.commit();
        return {item.id_, item.label_, DataRemovalItemStatus::Archived, *document, {}};
    }
    return deleteUnlinkedCv(item, *document, false, transaction);
}

DataRemovalItemOutcome DataRemovalService::restoreArchivedCv(const DataRemovalItemRequest& item) const
{
    SqlTransaction transaction{database_, QStringLiteral("restore archived CV")};
    auto document = cvRepository_.findById(item.id_);
    if (!document) {
        return failureOutcome(item, QStringLiteral("The CV was not found."));
    }
    if (!document->archivedAt_.isValid()) {
        return failureOutcome(item, QStringLiteral("The CV is already active."));
    }
    const auto updatedAt = cvRepository_.updateArchived(item.id_, false);
    if (!updatedAt) {
        return failureOutcome(item, QStringLiteral("The CV could not be restored."));
    }
    document->applyArchiveState(QDateTime{}, *updatedAt);
    transaction.commit();
    return {item.id_, item.label_, DataRemovalItemStatus::Restored, *document, {}};
}

DataRemovalItemOutcome DataRemovalService::deleteArchivedCv(const DataRemovalItemRequest& item) const
{
    SqlTransaction transaction{database_, QStringLiteral("permanently delete archived CV")};
    const auto document = cvRepository_.findById(item.id_);
    if (!document) {
        return failureOutcome(item, QStringLiteral("The CV was not found."));
    }
    if (!document->archivedAt_.isValid()) {
        return failureOutcome(item, QStringLiteral("The CV is not archived."));
    }
    if (!document->linkedApplicationIds_.isEmpty()) {
        return {
            item.id_,
            item.label_,
            DataRemovalItemStatus::SkippedLinked,
            *document,
            QStringLiteral("The CV is still used by one or more job applications.")};
    }
    return deleteUnlinkedCv(item, *document, true, transaction);
}

DataRemovalItemOutcome DataRemovalService::deleteUnlinkedCv(
    const DataRemovalItemRequest& item,
    const CvDocument& document,
    bool protectedLinkedCv,
    SqlTransaction& transaction) const
{
    const auto removal = managedFileStore_.prepareRemoval(document);
    if (!removal.succeeded()) {
        throw std::runtime_error(removal.message_.toStdString());
    }

    if (!cvRepository_.removeUnlinked(item.id_)) {
        const auto current = cvRepository_.findById(item.id_);
        if (current && !current->linkedApplicationIds_.isEmpty()) {
            if (protectedLinkedCv) {
                return {
                    item.id_,
                    item.label_,
                    DataRemovalItemStatus::SkippedLinked,
                    *current,
                    QStringLiteral("The CV became linked and was not deleted.")};
            }
            const auto updatedAt = cvRepository_.updateArchived(item.id_, true);
            if (updatedAt) {
                auto archived = *current;
                archived.applyArchiveState(*updatedAt, *updatedAt);
                transaction.commit();
                return {
                    item.id_,
                    item.label_,
                    DataRemovalItemStatus::Archived,
                    std::move(archived),
                    QStringLiteral("The CV became linked and was archived instead.")};
            }
        }
        return failureOutcome(item, QStringLiteral("The CV could not be deleted."));
    }

    transaction.commit();
    const bool cleaned = managedFileStore_.finalizeRemoval(*removal.preparation_);
    QString message;
    if (removal.fileWasMissing_) {
        message = QStringLiteral("The CV record was deleted; its managed file was already missing.");
    } else if (!cleaned) {
        message = QStringLiteral(
            "The CV record was deleted, but final file cleanup will be retried on restart.");
    }
    return {item.id_, item.label_, DataRemovalItemStatus::Deleted, document, message};
}

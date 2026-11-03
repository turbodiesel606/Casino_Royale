#include "DataRemovalService.hpp"

#include "common/CancellationState.hpp"
#include "common/ExceptionUtils.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "jobs/JobRepository.hpp"
#include "logging/ApplicationLog.hpp"
#include "storage/SqlTransaction.hpp"

#include <QDateTime>
#include <QScopeGuard>
#include <QSqlDatabase>

#include <exception>
#include <stdexcept>
#include <utility>

DataRemovalService::DataRemovalService(
	QSqlDatabase& database,
	JobRepository& jobRepository,
	CvRepository& cvRepository,
	CvManagedFileStore& managedFileStore)
	: database_{ database }
	, jobRepository_{ jobRepository }
	, cvRepository_{ cvRepository }
	, managedFileStore_{ managedFileStore }
{
}

void DataRemovalService::ItemFacts::commit(SqlTransaction& transaction)
{
	stage_ = LogFact::TransactionCommit;
	cancelledBeforeCommit_ = cancellationObservedBeforeCommit(cancellation_);
	transaction.commit();
}

void DataRemovalService::BatchFacts::observe(const ItemFacts& item, DataRemovalItemStatus status) noexcept
{
	auto& counts = item.cancelledBeforeCommit_ ? afterCancellation_ : ordinary_;
	switch (status) {
	case DataRemovalItemStatus::Deleted:
		++counts.deleted_;
		if (item.fileMissing_) ++counts.missing_;
		else if (item.cleanupPending_) ++counts.pending_;
		return;
	case DataRemovalItemStatus::Archived: ++counts.archived_; return;
	case DataRemovalItemStatus::Restored: ++counts.restored_; return;
	case DataRemovalItemStatus::SkippedLinked: ++protected_; return;
	case DataRemovalItemStatus::Failed: break;
	}
	if (item.mismatch_ != LogFact::None) {
		ApplicationLog::record({LogEventType::RequestedRecordStateMismatch, {item.operation_, item.mismatch_}, item.id_});
	} else if (item.invalidReference_ != LogFact::None) {
		ApplicationLog::record({LogEventType::InvalidManagedCvReferenceRejected,
			{LogFact::DeleteCv, item.invalidReference_}, item.id_});
	} else if (item.operation_ == LogFact::JobDeletion) {
		if (failedJobs_++ == 0) {
			firstJobFailureStage_ = item.stage_;
			firstJobFailureCause_ = item.cause_;
		}
	} else if (item.operation_ == LogFact::CvArchive) {
		ApplicationLog::record({LogEventType::LinkedCvArchiveFailed, {item.cause_}, item.id_});
	} else if (item.operation_ == LogFact::CvRestore) {
		ApplicationLog::record({LogEventType::ArchivedCvRestoreFailed, {item.stage_, item.cause_}, item.id_});
	} else if (item.operation_ == LogFact::ActiveCvRemoval) {
		// Codex Comment: Eligibility could not be read, so archive versus deletion is still unknown.
		ApplicationLog::record({LogEventType::BackgroundWorkStartFailed,
			{item.operation_, item.stage_, item.cause_, 1}, item.id_});
	} else {
		ApplicationLog::record({LogEventType::PermanentCvDeletionFailed,
			{item.stage_, item.cause_, item.restoration_}, item.id_});
	}
}

void DataRemovalService::BatchFacts::report(LogFact operation, quint64 id, qint64 selected,
	const DataRemovalBatchResult& result) const noexcept
{
	const auto recordCount = [id](LogEventType type, qint64 count) {
		if (count) ApplicationLog::record({type, {count}, id});
	};
	if (failedJobs_) {
		ApplicationLog::record({LogEventType::JobApplicationDeletionFailed,
			{firstJobFailureStage_, firstJobFailureCause_, ordinary_.deleted_ + afterCancellation_.deleted_, failedJobs_}, id});
	}
	if (protected_) {
		ApplicationLog::record({LogEventType::LinkedCvPermanentDeletionSkipped,
			{protected_ == selected ? LogFact::RemovalAllProtected : LogFact::RemovalSkipped, protected_, protected_}, id});
	}
	if (operation == LogFact::JobDeletion)
		recordCount(LogEventType::JobApplicationsDeleted, ordinary_.deleted_);
	else
		recordCount(LogEventType::UnlinkedCvsPermanentlyDeleted, ordinary_.deleted_ - ordinary_.missing_ - ordinary_.pending_);
	recordCount(LogEventType::LinkedActiveCvsArchived, ordinary_.archived_);
	recordCount(LogEventType::ArchivedCvsRestored, ordinary_.restored_);

	const auto& after = afterCancellation_;
	recordCount(LogEventType::DeletedCvFileAlreadyMissing, ordinary_.missing_ + after.missing_);
	recordCount(LogEventType::DeletedCvFileCleanupPending, ordinary_.pending_ + after.pending_);
	if (after.deleted_ || after.archived_ || after.restored_) {
		ApplicationLog::record({LogEventType::OperationCommittedAfterCancellationRequest,
			{operation, LogValue::counts(after.deleted_, after.archived_, after.restored_, after.missing_, after.pending_)}, id});
	}
	if (result.cancelled_) {
		ApplicationLog::record({LogEventType::OperationStoppedByCancellation,
			{operation, LogFact::RemovalBetweenItems, LogValue::counts(result.items_.size(), selected - result.items_.size())}, id});
	}
}

DataRemovalItemOutcome DataRemovalService::deleteJob(const DataRemovalItemRequest& item, ItemFacts& facts) const
{
	/*
	opening new transaction is reduntant for single query...
	we do only one operation(DELETE).
	try without SqlTransaction while do refactor.

	current behavior is correct!
	*/

	SqlTransaction transaction{ database_, QStringLiteral("delete job application"), facts.operation_, nullptr, facts.id_ };
	facts.stage_ = LogFact::RemovalDeleteRow;
	if (!jobRepository_.remove(item.id_)) {
		facts.mismatch_ = LogFact::Missing;
		return failureOutcome(item, QStringLiteral("The job application was not found."));
	}

	facts.commit(transaction);
	return { item.id_, item.label_, DataRemovalItemStatus::Deleted, {}, {} };
}

DataRemovalItemOutcome DataRemovalService::removeActiveCv(const DataRemovalItemRequest& item, ItemFacts& facts) const
{
	SqlTransaction transaction{ database_, QStringLiteral("remove CV from library"), facts.operation_, nullptr, facts.id_ };
	facts.stage_ = LogFact::RemovalLookup;
	auto document = cvRepository_.findById(item.id_);

	if (!document) {
		facts.mismatch_ = LogFact::Missing;
		return failureOutcome(item, QStringLiteral("The CV was not found."));
	}

	if (document->isArchived_) {
		facts.mismatch_ = LogFact::RemovalAlreadyArchived;
		return failureOutcome(item, QStringLiteral("The CV is already archived."));
	}

	// If CV is linked, archive it
	if (!document->linkedApplicationIds_.isEmpty()) {

		facts.operation_ = LogFact::CvArchive;
		const auto updatedAt = cvRepository_.updateArchived(item.id_, true);

		if (!updatedAt)
			return failureOutcome(item, QStringLiteral("The CV could not be archived."));

		document->applyArchiveState(true, *updatedAt, *updatedAt);
		facts.commit(transaction);
		return { item.id_, item.label_, DataRemovalItemStatus::Archived, *document, {} };
	}

	return deleteUnlinkedCv(item, *document, transaction, facts);
}

DataRemovalItemOutcome DataRemovalService::restoreArchivedCv(const DataRemovalItemRequest& item, ItemFacts& facts) const
{
	// if CV is archived -> restore, otherwise return with message.
	SqlTransaction transaction{ database_, QStringLiteral("restore archived CV"), facts.operation_, nullptr, facts.id_ };
	facts.stage_ = LogFact::RemovalLookup;
	auto document = cvRepository_.findById(item.id_);

	if (!document) {
		facts.mismatch_ = LogFact::Missing;
		return failureOutcome(item, QStringLiteral("The CV was not found."));
	}

	// Archive membership does not depend on timestamp parsing success.
	if (!document->isArchived_) {
		facts.mismatch_ = LogFact::RemovalAlreadyActive;
		return failureOutcome(item, QStringLiteral("The CV is already active."));
	}

	facts.stage_ = LogFact::RemovalRestoreRow;
	const auto updatedAt = cvRepository_.updateArchived(item.id_, false);

	if (!updatedAt) 
		return failureOutcome(item, QStringLiteral("The CV could not be restored."));
	
	document->applyArchiveState(false, QDateTime{}, *updatedAt);
	facts.commit(transaction);
	return { item.id_, item.label_, DataRemovalItemStatus::Restored, *document, {} };
}

DataRemovalItemOutcome DataRemovalService::deleteArchivedCv(const DataRemovalItemRequest& item, ItemFacts& facts) const
{
	SqlTransaction transaction{ database_, QStringLiteral("permanently delete archived CV"), facts.operation_, nullptr, facts.id_ };
	facts.stage_ = LogFact::RemovalLookup;
	const auto document = cvRepository_.findById(item.id_);

	if (!document) {
		facts.mismatch_ = LogFact::Missing;
		return failureOutcome(item, QStringLiteral("The CV was not found."));
	}
	
	if (!document->isArchived_) {
		facts.mismatch_ = LogFact::RemovalAlreadyActive;
		return failureOutcome(item, QStringLiteral("The CV is not archived."));
	}
	
	if (!document->linkedApplicationIds_.isEmpty()) {
		return {
			item.id_,
			item.label_,
			DataRemovalItemStatus::SkippedLinked,
			*document,
			QStringLiteral("The CV is still used by one or more job applications.") };
	}

	return deleteUnlinkedCv(item, *document, transaction, facts);
}

DataRemovalItemOutcome DataRemovalService::deleteUnlinkedCv(
	const DataRemovalItemRequest& item,
	const CvDocument& document,
	SqlTransaction& transaction, ItemFacts& facts) const
{
	facts.operation_ = LogFact::CvDeletion;
	facts.stage_ = LogFact::RemovalStageFile;
	const auto removal = managedFileStore_.prepareRemoval(document);
	if (removal.preparation_)
		removal.preparation_->operationId_ = facts.id_;
	// Codex Comment: Observe the existing compensation before the caller rolls back. The shared preparation owns no observer.
	const auto restore = qScopeGuard([&] {
		if (removal.preparation_)
			facts.restoration_ = removal.preparation_->restoreRetainedFile();
	});
	if (!removal.succeeded()) {
		if (removal.failure_ == LogFact::StorageUnavailable || removal.failure_ == LogFact::RemovalRenamingFailed)
			facts.cause_ = removal.failure_;
		else
			facts.invalidReference_ = removal.failure_;
		throw std::runtime_error(removal.message_.toStdString());
	}
	

	facts.stage_ = LogFact::RemovalDeleteRow;
	if (!cvRepository_.removeUnlinked(item.id_)) {
		return failureOutcome(
			item,
			QStringLiteral("The CV record in SQL could not be deleted."));
	}

	facts.commit(transaction);

	const bool cleaned = managedFileStore_.finalizeRemoval(*removal.preparation_);
	facts.fileMissing_ = removal.fileWasMissing_;
	facts.cleanupPending_ = !cleaned;
	QString message;

	if (removal.fileWasMissing_)
		message = QStringLiteral("The CV record was deleted; its managed file was already missing.");
	
	else if (!cleaned) 
		message = QStringLiteral(
			"The CV record was deleted, but final file cleanup will be retried on restart.");
	
	return { item.id_, item.label_, DataRemovalItemStatus::Deleted, document, std::move(message) };
}

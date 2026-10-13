#include "StorageMutationGate.hpp"

#include <QStringList>
#include <QThread>

#include <limits>

StorageMutationGate::StorageMutationGate(QObject* parent)
	: QObject{ parent }
{
}

bool StorageMutationGate::reserveImportOrUpdate(int count)
{
	assertGuiThread();
	Q_ASSERT(count >= 0 && count <= std::numeric_limits<int>::max() - pendingImportsOrUpdates_);
	if (count < 0 || count > std::numeric_limits<int>::max() - pendingImportsOrUpdates_) {
		return false;
	}
	if (count == 0) {
		return true;
	}
	if (!checkImportOrUpdateAllowed()) {
		return false;
	}
	pendingImportsOrUpdates_ += count;
	clearRetryNotice<OperationCategory::ImportOrUpdate>();
	publishStateChange();
	return true;
}

void StorageMutationGate::releaseImportOrUpdate(int count)
{
	assertGuiThread();
	Q_ASSERT(count >= 0 && count <= pendingImportsOrUpdates_);
	if (count <= 0 || count > pendingImportsOrUpdates_) {
		return;
	}
	pendingImportsOrUpdates_ -= count;
	publishStateChange();
}

bool StorageMutationGate::beginRemoval()
{
	assertGuiThread();
	if (!checkRemovalAllowed())
		return false;

	removalActive_ = true;

	/*
	clear the old retry notification for deletion, archiving, or restoration 
	once a new removal request has been accepted.
	*/
	clearRetryNotice<OperationCategory::Removal>();
	publishStateChange();
	return true;
}

void StorageMutationGate::endRemoval()
{
	assertGuiThread();
	if (!removalActive_) {
		return;
	}
	removalActive_ = false;
	publishStateChange();
}

bool StorageMutationGate::checkImportOrUpdateAllowed()
{
	return checkAllowed<OperationCategory::ImportOrUpdate>();
}

bool StorageMutationGate::checkRemovalAllowed()
{
	return checkAllowed<OperationCategory::Removal>();
}

bool StorageMutationGate::canBeginImportOrUpdate() const
{
	return blockingReason<OperationCategory::ImportOrUpdate>() == BlockReason::None;
}

bool StorageMutationGate::removalActive() const
{
	return removalActive_;
}

bool StorageMutationGate::canBeginRemoval() const
{
	return blockingReason<OperationCategory::Removal>() == BlockReason::None;
}

bool StorageMutationGate::shuttingDown() const
{
	return shuttingDown_;
}

#if 0
StorageMutationGate::BlockReason StorageMutationGate::blockingReason(OperationCategory category) const
{
	if (shuttingDown_)
		return BlockReason::Closing;

	if (removalActive_)
		return BlockReason::RemovalActive;

	if (category == OperationCategory::Removal && pendingImportsOrUpdates_ > 0)
		return BlockReason::ImportsOrUpdatesPending;

	return BlockReason::None;
}
#endif // 0
#if 0


bool StorageMutationGate::checkAllowed(OperationCategory category)
{
	assertGuiThread();
	const auto reason = blockingReason(category);
	if (reason == BlockReason::None) {
		return true;
	}
	if (reason != BlockReason::Closing) {
		lastBlockedCategory_ = category;
		retryNotices_[static_cast<std::size_t>(category)].requested_ = true;
		emit feedbackChanged();
		// This event is intentional even when neither category nor reason changed.
		emit operationBlocked(category, reason);
	}
	return false;
}
#endif // 0

void StorageMutationGate::publishStateChange()
{
	for (const auto category : { OperationCategory::ImportOrUpdate, OperationCategory::Removal }) {
		auto& notice = retryNotices_[static_cast<std::size_t>(category)];
		if (notice.requested_ && blockingReason(category) == BlockReason::None)
			notice.announced_ = true;
	}

	// Notify observers to reevaluate mutation availability and shutdown state.
	emit stateChanged();

	/*
	Notify QML bindings to refresh retry banner visibility / text
	and the blocked-operation dialog's title/message. 
	*/
	emit feedbackChanged(); 
}

bool StorageMutationGate::retryNoticeVisible() const
{
	return !shuttingDown_ && (retryNotices_[0].announced_ || retryNotices_[1].announced_);
}

QString StorageMutationGate::retryNoticeMessage() const
{
	if (!retryNoticeVisible()) {
		return {};
	}
	QStringList messages;
	if (retryNotices_[0].requested_) {
		messages.append(canBeginImportOrUpdate()
			? tr("The blocking work has finished. You can try importing or saving again.")
			: tr("Deletion, archiving, or restoration is running. Importing and saving are blocked again."));
	}
	if (retryNotices_[1].requested_) {
		messages.append(canBeginRemoval()
			? tr("The blocking work has finished. You can try deleting, archiving, or restoring again.")
			: tr("Work is running again. Wait before trying to delete, archive, or restore."));
	}
	return messages.join(QLatin1Char('\n'));
}

QString StorageMutationGate::blockedTitle() const
{
	switch (blockingReason(lastBlockedCategory_)) {
	case BlockReason::ImportsOrUpdatesPending:
		return tr("Imports and updates are still running");
	case BlockReason::RemovalActive:
		return tr("Deletion, archiving, or restoration is still running");
	case BlockReason::Closing:
		return tr("The application is closing");
	case BlockReason::None:
		return tr("The blocking work has finished");
	}
	return {};
}

QString StorageMutationGate::blockedMessage() const
{
	if (blockingReason(lastBlockedCategory_) == BlockReason::None) {
		return tr("Your request was not started. Close this dialog and try again when you are ready.");
	}
	return tr("Your request was not started. Wait for the current work to finish. "
		"We'll notify you when you can try again.");
}

void StorageMutationGate::dismissRetryNotice()
{
	assertGuiThread();
	retryNotices_ = {};
	emit feedbackChanged();
}

void StorageMutationGate::beginShutdown()
{
	assertGuiThread();
	if (shuttingDown_)
		return;
	
	shuttingDown_ = true;
	retryNotices_ = {};
	publishStateChange();
}

void StorageMutationGate::assertGuiThread() const
{
	Q_ASSERT(QThread::currentThread() == thread());
}

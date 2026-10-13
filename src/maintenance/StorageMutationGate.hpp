#ifndef JOBTRACKER_SRC_MAINTENANCE_STORAGEMUTATIONGATE_HPP
#define JOBTRACKER_SRC_MAINTENANCE_STORAGEMUTATIONGATE_HPP

#include <QObject>
#include <QString>

#include <array>

#include <chrono>
#include <thread>

/*
	GUI - thread admission for accepted mutations,
	including queued work and result publication.
	Retry notices track user intent, never requests to execute later.
*/

class StorageMutationGate final : public QObject
{
	Q_OBJECT
		Q_PROPERTY(bool importOrUpdateAllowed READ canBeginImportOrUpdate NOTIFY stateChanged)
		Q_PROPERTY(bool removalAllowed READ canBeginRemoval NOTIFY stateChanged)
		Q_PROPERTY(bool shuttingDown READ shuttingDown NOTIFY stateChanged)
		Q_PROPERTY(bool retryNoticeVisible READ retryNoticeVisible NOTIFY feedbackChanged)
		Q_PROPERTY(QString retryNoticeMessage READ retryNoticeMessage NOTIFY feedbackChanged)
		Q_PROPERTY(QString blockedTitle READ blockedTitle NOTIFY feedbackChanged)
		Q_PROPERTY(QString blockedMessage READ blockedMessage NOTIFY feedbackChanged)

public:
	enum class OperationCategory { ImportOrUpdate, Removal };
	Q_ENUM(OperationCategory)
		enum class BlockReason { None, ImportsOrUpdatesPending, RemovalActive, Closing };
	Q_ENUM(BlockReason)

		explicit StorageMutationGate(QObject* parent = nullptr);

	bool reserveImportOrUpdate(int count = 1);
	void releaseImportOrUpdate(int count = 1);
	bool beginRemoval();
	void endRemoval();

	Q_INVOKABLE bool checkImportOrUpdateAllowed();
	Q_INVOKABLE bool checkRemovalAllowed();
	Q_INVOKABLE void dismissRetryNotice();
	Q_INVOKABLE void beginShutdown();

	bool canBeginImportOrUpdate() const;
	bool removalActive() const;
	bool canBeginRemoval() const;
	bool shuttingDown() const;
	bool retryNoticeVisible() const;
	QString retryNoticeMessage() const;
	QString blockedTitle() const;
	QString blockedMessage() const;

signals:
	void stateChanged();
	void feedbackChanged();
	void operationBlocked(OperationCategory category, BlockReason reason);

private:
	struct RetryNotice final
	{
		bool requested_ = false;
		bool announced_ = false;
	};
	// template needed for microoptimization. Yes, it is stupid...
	template<OperationCategory Category>
	StorageMutationGate::BlockReason blockingReason() const
	{
		if (shuttingDown_)
			return BlockReason::Closing;

		if (removalActive_)
			return BlockReason::RemovalActive;

		if constexpr (Category == OperationCategory::Removal) {
			if (pendingImportsOrUpdates_ > 0)
				return BlockReason::ImportsOrUpdatesPending;
		}

		return BlockReason::None;
	}

	constexpr BlockReason blockingReason(OperationCategory category) const
	{
		switch (category) {
		case OperationCategory::ImportOrUpdate:
			return blockingReason<OperationCategory::ImportOrUpdate>();
		case OperationCategory::Removal:
			return blockingReason<OperationCategory::Removal>();
		}
	}

	template<OperationCategory Category>
	bool checkAllowed()
	{
		assertGuiThread();
		const auto reason = blockingReason(Category);
		if (reason == BlockReason::None)
			return true;

		// Suppress new dialogs and retry notices during shutdown.
		if (reason != BlockReason::Closing) {
			/*
			Remember which operation was blocked so the UI can explain
			the conflict and later announce when that operation becomes available.
			*/
			// This stores notification interest, not a request for automatic retry.
			lastBlockedCategory_ = Category;
			retryNotices_[static_cast<std::size_t>(Category)].requested_ = true;
			// Refresh QML bindings that display the feedback state.
			emit feedbackChanged();

			/*
				Report every rejected attempt so the UI can show the
				blocking dialog again, even when the category and reason are unchanged.
			*/
			emit operationBlocked(Category, reason);
		}
		return false;
	}

	void publishStateChange();

	template<OperationCategory Category>
	void clearRetryNotice()
	{
		retryNotices_[static_cast<std::size_t>(Category)] = {};
	}
	void assertGuiThread() const;

	int pendingImportsOrUpdates_ = 0;
	bool removalActive_ = false;
	bool shuttingDown_ = false;
	OperationCategory lastBlockedCategory_ = OperationCategory::ImportOrUpdate;
	std::array<RetryNotice, 2> retryNotices_{};
};

#endif // JOBTRACKER_SRC_MAINTENANCE_STORAGEMUTATIONGATE_HPP

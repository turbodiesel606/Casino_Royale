#ifndef JOBTRACKER_SRC_MAINTENANCE_STORAGEMUTATIONGATE_HPP
#define JOBTRACKER_SRC_MAINTENANCE_STORAGEMUTATIONGATE_HPP

#include "logging/LogEvent.hpp"

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

	bool reserveImportOrUpdate(int count = 1, LogFact action = LogFact::ImportOrUpdate);
	void releaseImportOrUpdate(int count = 1);
	bool beginRemoval(LogFact action = LogFact::RemovalWork);
	void endRemoval();

	static void recordCancellation(StorageMutationGate* gate, int active, int queued) noexcept;
	qint64 shutdownActiveCancellationCount() const { return shutdownActiveCancellationCount_; }
	qint64 shutdownQueuedCancellationCount() const { return shutdownQueuedCancellationCount_; }

	Q_INVOKABLE bool checkImportOrUpdateAllowed();
	Q_INVOKABLE bool checkRemovalAllowed();
	Q_INVOKABLE bool checkCvImportAllowed();
	Q_INVOKABLE bool checkJobDeletionAllowed();
	Q_INVOKABLE bool checkActiveCvRemovalAllowed();
	Q_INVOKABLE bool checkArchivedCvDeletionAllowed();
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
	BlockReason blockingReason(OperationCategory category) const;
	bool checkAllowed(OperationCategory category, LogFact action);
	void clearRetryNotice(OperationCategory category);
	void publishStateChange();
	void recordBlockedAttempt(LogFact action, OperationCategory category, BlockReason reason) const noexcept;
	void assertGuiThread() const;

	int pendingImportsOrUpdates_ = 0;
	bool removalActive_ = false;
	LogFact activeRemovalAction_ = LogFact::RemovalWork;
	qint64 shutdownActiveCancellationCount_ = 0;
	qint64 shutdownQueuedCancellationCount_ = 0;
	bool shuttingDown_ = false;
	OperationCategory lastBlockedCategory_ = OperationCategory::ImportOrUpdate;
	std::array<RetryNotice, 2> retryNotices_{};
};

#endif // JOBTRACKER_SRC_MAINTENANCE_STORAGEMUTATIONGATE_HPP

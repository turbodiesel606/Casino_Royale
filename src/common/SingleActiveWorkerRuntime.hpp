#ifndef JOBTRACKER_SRC_COMMON_SINGLEACTIVEWORKERRUNTIME_HPP
#define JOBTRACKER_SRC_COMMON_SINGLEACTIVEWORKERRUNTIME_HPP

#include "CancellationState.hpp"

#include <QObject>
#include <QString>
#include <QThread>
#include <QtGlobal>

#include <functional>
#include <memory>

enum class SingleActiveWorkerAdmission
{
	Accepted,
	Busy,
	ShuttingDown,
};

/**
 * Type-erased lifecycle for one domain-specific QObject executor.
 *
 * create_ runs lazily on the facade thread and must return an unparented
 * QObject. destroyContext_ runs synchronously on the executor thread before
 * that thread is stopped, allowing SQL and other thread-affine state to be
 * released correctly.
 */
struct SingleActiveWorkerExecutorLifecycle final
{
	std::function<QObject* ()> create_;
	std::function<void(QObject&)> destroyContext_;
	/* stub param (QObject&) is needed for compatibility with other caller code,
	   see e.g. DataRemovalWorker::DataRemovalWorker() */
};

/**
 * Compositional runtime for GUI-thread facades with one reusable worker thread
 * and at most one correlated operation.
 *
 * Concrete facades keep their typed requests, outcomes, signals, executor
 * processing, error messages, and queued result delivery. This class owns only
 * executor/thread lifetime, busy/shutdown state, cooperative cancellation, and
 * stale-result correlation.
 */
class SingleActiveWorkerRuntime final
{
public:
	SingleActiveWorkerRuntime(
		QString threadName,
		SingleActiveWorkerExecutorLifecycle executorLifecycle);
	~SingleActiveWorkerRuntime();

	SingleActiveWorkerRuntime(const SingleActiveWorkerRuntime&) = delete;
	SingleActiveWorkerRuntime& operator=(const SingleActiveWorkerRuntime&) = delete;
	SingleActiveWorkerRuntime(SingleActiveWorkerRuntime&&) = delete;
	SingleActiveWorkerRuntime& operator=(SingleActiveWorkerRuntime&&) = delete;

	/**
	 * Lazily creates and starts the executor before publishing busy state.
	 * Factory failures leave the runtime idle and propagate to the facade.
	 */
	SingleActiveWorkerAdmission tryBeginOperation(
		quint64 operationId,
		std::shared_ptr<CancellationState> cancellation);

	QObject& executor() const;
	bool isActiveOutcome(
		quint64 operationId,
		const std::shared_ptr<CancellationState>& cancellation) const;
	bool isBusy() const;
	bool isRunning() const;
	bool isShuttingDown() const;

	bool requestActiveCancellation();
	void completeOperation();

	/**
	 * Idempotently cancels active work, destroys domain context on the worker
	 * thread, quits and joins that thread, and clears active correlation.
	 */
	void shutdown();

private:
	void assertFacadeThread() const;
	void ensureExecutor();
	void clearActiveOperation();

	SingleActiveWorkerExecutorLifecycle executorLifecycle_;
	QThread* facadeThread_ = nullptr;
	QThread workerThread_;
	QObject* executor_ = nullptr;
	std::shared_ptr<CancellationState> activeCancellation_;
	quint64 activeOperationId_ = 0;
	bool busy_ = false;
	bool shuttingDown_ = false;
};

#endif // JOBTRACKER_SRC_COMMON_SINGLEACTIVEWORKERRUNTIME_HPP

#ifndef JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALSERVICE_HPP
#define JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALSERVICE_HPP

#include "DataRemovalTypes.hpp"
#include "common/CancellationState.hpp"
#include "common/ExceptionUtils.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "jobs/JobRepository.hpp"
#include "storage/SqlTransaction.hpp"
#include "storage/SqlQuery.hpp"

#include <utility>
class CancellationState;
class CvManagedFileStore;
class CvRepository;
class JobRepository;
class QSqlDatabase;
class SqlTransaction;

namespace {

	inline DataRemovalItemOutcome failureOutcome(
		const DataRemovalItemRequest& item,
		QString message)
	{
		return { item.id_, item.label_, DataRemovalItemStatus::Failed, {}, std::move(message) };
	}

}

// Coordinates best-effort per-item job/CV removal transactions and managed
// file compensation. The service and all dependencies stay on one SQL thread.
class DataRemovalService final
{
public:
	DataRemovalService(
		QSqlDatabase& database,
		JobRepository& jobRepository,
		CvRepository& cvRepository,
		CvManagedFileStore& managedFileStore);

	template<DataRemovalKind Kind>
		requires SupportedDataRemovalKind<Kind>
	DataRemovalBatchResult process(
		const QVector<DataRemovalItemRequest>& items,
		const std::shared_ptr<CancellationState>& cancellation,
		quint64 operationId = 0) const
	{
		DataRemovalBatchResult result;
		result.items_.reserve(items.size());
		BatchFacts batch;

		for (const auto& item : items) {
			/* Cancellation stops before the next item work. */

			if (cancellation != nullptr && cancellation->isCancellationRequested()) {
				result.cancelled_ = true;
				break;
			}

			ItemFacts facts{removalLogOperation(Kind), operationId, cancellation};
			try {
				result.items_.append([this, &item, &facts]() {
					if constexpr (Kind == DataRemovalKind::DeleteJobs)
						return deleteJob(item, facts);

					else if constexpr (Kind == DataRemovalKind::RemoveActiveCvs)
						return removeActiveCv(item, facts);

					else if constexpr (Kind == DataRemovalKind::RestoreArchivedCvs)
						return restoreArchivedCv(item, facts);

					else
						return deleteArchivedCv(item, facts);

					}());
			}
			catch (...) {
				if (facts.cause_ == LogFact::CauseUnavailable)
					facts.cause_ = storage::sql::currentFailureCause();
				result.items_.append(
					failureOutcome(
						item,
						common::exceptionMessage(
							std::current_exception(),
							QStringLiteral(
								"An unexpected deletion error occurred."))));
			}
			batch.observe(facts, result.items_.last().status_);
		}
		batch.report(removalLogOperation(Kind), operationId, items.size(), result);
		return result;
	}

private:
	struct ItemFacts
	{
		LogFact operation_;
		quint64 id_;
		const std::shared_ptr<CancellationState>& cancellation_;
		LogFact stage_ = LogFact::TransactionBegin;
		LogFact cause_ = LogFact::CauseUnavailable;
		LogFact lookupReadFailure_ = LogFact::None;
		LogFact mismatch_ = LogFact::None;
		LogFact invalidReference_ = LogFact::None;
		LogFact restoration_ = LogFact::NotAttempted;
		bool fileMissing_ = false;
		bool cleanupPending_ = false;
		bool cancelledBeforeCommit_ = false;

		void commit(SqlTransaction& transaction);
	};
	struct BatchFacts
	{
		LogRemovalCounts ordinary_;
		LogRemovalCounts afterCancellation_;
		qint64 protected_ = 0;
		qint64 missingJobs_ = 0;
		LogFact firstJobFailureStage_ = LogFact::Unknown;
		LogFact firstJobFailureCause_ = LogFact::CauseUnavailable;
		qint64 activeCvFailures_ = 0;
		LogFact activeCvFailureStage_ = LogFact::Unknown;
		LogFact activeCvFailureCause_ = LogFact::CauseUnavailable;
		void reportActiveCvFailures(quint64 id) const noexcept;
		void observeActiveCvFailure(LogFact stage, LogFact cause, quint64 id) noexcept;
		void observe(const ItemFacts& item, DataRemovalItemStatus status) noexcept;
		void report(LogFact operation, quint64 id, qint64 selected, const DataRemovalBatchResult& result) const noexcept;
	};
	DataRemovalItemOutcome deleteJob(const DataRemovalItemRequest& item, ItemFacts& facts) const;
	DataRemovalItemOutcome removeActiveCv(const DataRemovalItemRequest& item, ItemFacts& facts) const;
	DataRemovalItemOutcome restoreArchivedCv(const DataRemovalItemRequest& item, ItemFacts& facts) const;
	DataRemovalItemOutcome deleteArchivedCv(const DataRemovalItemRequest& item, ItemFacts& facts) const;
	DataRemovalItemOutcome deleteUnlinkedCv(
		const DataRemovalItemRequest& item,
		const CvDocument& document,
		SqlTransaction& transaction, ItemFacts& facts) const;

	QSqlDatabase& database_;
	JobRepository& jobRepository_;
	CvRepository& cvRepository_;
	CvManagedFileStore& managedFileStore_;
};

#endif // JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALSERVICE_HPP

#ifndef JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALSERVICE_HPP
#define JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALSERVICE_HPP

#include "DataRemovalTypes.hpp"
#include "DataRemovalService.hpp"

#include "common/CancellationState.hpp"
#include "common/ExceptionUtils.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "jobs/JobRepository.hpp"
#include "storage/SqlTransaction.hpp"
#include<thread>
#include<chrono>
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
		const std::shared_ptr<CancellationState>& cancellation) const
	{
		DataRemovalBatchResult result;
		result.items_.reserve(items.size());

		for (const auto& item : items) {
			/* Cancellation stops before the next item work. */

			if (cancellation != nullptr && cancellation->isCancellationRequested()) {
				result.cancelled_ = true;
				break;
			}

			try {
				result.items_.append([this, &item]() {
					if constexpr (Kind == DataRemovalKind::DeleteJobs)
						return deleteJob(item);

					else if constexpr (Kind == DataRemovalKind::RemoveActiveCvs)
						return removeActiveCv(item);

					else if constexpr (Kind == DataRemovalKind::RestoreArchivedCvs)
						return restoreArchivedCv(item);

					else
						return deleteArchivedCv(item);

					}());
			}
			catch (...) {
				result.items_.append(
					failureOutcome(
						item,
						common::exceptionMessage(
							std::current_exception(),
							QStringLiteral(
								"An unexpected deletion error occurred."))));
			}
		}

		return result;
	}

private:
	DataRemovalItemOutcome deleteJob(const DataRemovalItemRequest& item) const;
	DataRemovalItemOutcome removeActiveCv(const DataRemovalItemRequest& item) const;
	DataRemovalItemOutcome restoreArchivedCv(const DataRemovalItemRequest& item) const;
	DataRemovalItemOutcome deleteArchivedCv(const DataRemovalItemRequest& item) const;
	DataRemovalItemOutcome deleteUnlinkedCv(
		const DataRemovalItemRequest& item,
		const CvDocument& document,
		SqlTransaction& transaction) const;

	QSqlDatabase& database_;
	JobRepository& jobRepository_;
	CvRepository& cvRepository_;
	CvManagedFileStore& managedFileStore_;
};

#endif // JOBTRACKER_SRC_MAINTENANCE_DATAREMOVALSERVICE_HPP

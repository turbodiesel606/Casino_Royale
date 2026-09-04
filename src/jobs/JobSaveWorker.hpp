#ifndef JOBTRACKER_SRC_JOBS_JOBSAVEWORKER_HPP
#define JOBTRACKER_SRC_JOBS_JOBSAVEWORKER_HPP

#include "JobSaveExecutor.hpp"
#include "JobSaveWorkerTypes.hpp"
#include "common/SingleActiveWorkerFacade.hpp"

#include <QLatin1StringView>
#include <QMetaType>
#include <QObject>
#include <QString>

#include <utility>

struct JobSaveWorkerRequestMessages
{
	static constexpr auto errorMessage =
		QLatin1StringView{"An unexpected job-save worker error occurred."};

	static constexpr auto shuttingDownMessage =
		QLatin1StringView{"The job-save worker is shutting down."};

	static constexpr auto busyMessage =
		QLatin1StringView{"The job-save worker already has an active request."};
};

template<>
struct SingleActiveWorkerRequestTraits<AddJobRequest>
	: JobSaveWorkerRequestMessages
{
	static AddJobSaveOutcome unavailableOutcome(
		const AddJobRequest& request,
		QString message)
	{
		AddJobResult result;
		result.message_ = std::move(message);

		return {
			request.operationId_,
			request.cancellation_,
			request.draft_.jobTitle_,
			std::move(result) };
	}
};

template<>
struct SingleActiveWorkerRequestTraits<UpdateJobRequest>
	: JobSaveWorkerRequestMessages
{
	static UpdateJobSaveOutcome unavailableOutcome(
		const UpdateJobRequest& request,
		QString message)
	{
		UpdateJobResult result;
		result.message_ = std::move(message);

		return {
			request.operationId_,
			request.cancellation_,
			request.applicationId_,
			request.draft_.jobTitle_,
			std::move(result) };
	}
};

// GUI-thread facade for one reusable job-save worker thread.
class JobSaveWorker final
	: public SingleActiveWorkerFacade<JobSaveExecutor>
{
	Q_OBJECT

	using Base = SingleActiveWorkerFacade<JobSaveExecutor>;

public:
	explicit JobSaveWorker(
		QString dataDirectory,
		QObject* parent = nullptr);

signals:
	void saveCompleted(const AddJobSaveOutcome& outcome);
	void updateCompleted(const UpdateJobSaveOutcome& outcome);
};

#endif // JOBTRACKER_SRC_JOBS_JOBSAVEWORKER_HPP

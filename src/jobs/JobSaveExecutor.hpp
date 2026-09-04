#ifndef JOBTRACKER_SRC_JOBS_JOBSAVEEXECUTOR_HPP
#define JOBTRACKER_SRC_JOBS_JOBSAVEEXECUTOR_HPP

#include "JobSaveWorkerTypes.hpp"

#include <QObject>
#include <QString>
#include <QVariantMap>

#include <memory>
#include <variant>

class JobSaveExecutor final : public QObject
{
public:
	using Outcome = std::variant<
		AddJobSaveOutcome,
		UpdateJobSaveOutcome>;

	explicit JobSaveExecutor(QString dataDirectory);
	~JobSaveExecutor() override;

	AddJobSaveOutcome process(AddJobRequest request);
	UpdateJobSaveOutcome process(UpdateJobRequest request);

	void destroyContext();

private:
	struct PipelineContext;

	PipelineContext& ensureContext();

	static AddJobResult addFailure(
		QString message,
		QVariantMap fieldErrors = {});
	static UpdateJobResult updateFailure(
		QString message,
		QVariantMap fieldErrors = {});
	static AddJobSaveOutcome saveOutcome(
		const AddJobRequest& request,
		AddJobResult result);
	static UpdateJobSaveOutcome updateOutcome(
		const UpdateJobRequest& request,
		UpdateJobResult result);

	QString dataDirectory_;
	std::unique_ptr<PipelineContext> context_;
};

#endif // JOBTRACKER_SRC_JOBS_JOBSAVEEXECUTOR_HPP

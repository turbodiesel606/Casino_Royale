#ifndef JOBTRACKER_SRC_JOBS_JOBSAVEWORKERTYPES_HPP
#define JOBTRACKER_SRC_JOBS_JOBSAVEWORKERTYPES_HPP

#include "AddJobService.hpp"
#include "JobApplicationDraft.hpp"
#include "UpdateJobService.hpp"
#include "common/CancellationState.hpp"

#include <QMetaType>
#include <QString>
#include <QUrl>
#include <QtGlobal>

#include <memory>

// Carries one controller-validated Add Job request into the worker thread.
struct AddJobRequest final
{
	quint64 operationId_ = 0;
	NormalizedJobApplicationDraft draft_;
	QUrl selectedCvUrl_;
	std::shared_ptr<CancellationState> cancellation_;
};

// Returns the value-only durable result after worker-side CV and SQL work.
struct AddJobSaveOutcome final
{
	quint64 operationId_ = 0;
	std::shared_ptr<CancellationState> cancellation_;
	QString jobTitle_;
	AddJobResult result_;
};

Q_DECLARE_METATYPE(AddJobSaveOutcome)

struct UpdateJobRequest final
{
	quint64 operationId_ = 0;
	QString applicationId_;
	NormalizedJobApplicationDraft draft_;
	QUrl replacementCvUrl_;
	std::shared_ptr<CancellationState> cancellation_;
};

struct UpdateJobSaveOutcome final
{
	quint64 operationId_ = 0;
	std::shared_ptr<CancellationState> cancellation_;
	QString applicationId_;
	QString jobTitle_;
	UpdateJobResult result_;
};

Q_DECLARE_METATYPE(UpdateJobSaveOutcome)

#endif // JOBTRACKER_SRC_JOBS_JOBSAVEWORKERTYPES_HPP

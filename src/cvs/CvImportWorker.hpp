#ifndef JOBTRACKER_SRC_CVS_CVIMPORTWORKER_HPP
#define JOBTRACKER_SRC_CVS_CVIMPORTWORKER_HPP

#include "CvImportExecutor.hpp"
#include "CvImportWorkerTypes.hpp"
#include "common/SingleActiveWorkerFacade.hpp"

#include <QFileInfo>
#include <QLatin1StringView>
#include <QMetaType>
#include <QObject>
#include <QString>

#include <utility>

struct CvImportWorkerRequestMessages
{
	static constexpr auto errorMessage =
		QLatin1StringView{"An unexpected CV import error occurred."};

	static constexpr auto shuttingDownMessage =
		QLatin1StringView{"The CV import worker is shutting down."};

	static constexpr auto busyMessage =
		QLatin1StringView{"The CV import worker already has an active request."};
};

template<>
struct SingleActiveWorkerRequestTraits<CvImportRequest>
	: CvImportWorkerRequestMessages
{
	static CvImportSaveOutcome unavailableOutcome(
		const CvImportRequest& request,
		QString message)
	{
		const auto selectedFileName =
			QFileInfo{ request.sourceUrl_.toLocalFile() }.fileName();

		CvImportSaveOutcome outcome;
		outcome.operationId_ = request.operationId_;
		outcome.cancellation_ = request.cancellation_;
		outcome.fileName_ = selectedFileName.isEmpty()
			? QStringLiteral("Selected CV")
			: selectedFileName;
		outcome.message_ = std::move(message);
		return outcome;
	}
};

// GUI-thread facade for one reusable CV import worker thread.
class CvImportWorker final
	: public SingleActiveWorkerFacade<CvImportExecutor>
{
	Q_OBJECT

	using Base = SingleActiveWorkerFacade<CvImportExecutor>;

public:
	explicit CvImportWorker(
		QString dataDirectory,
		QObject* parent = nullptr);

signals:
	void importCompleted(const CvImportSaveOutcome& outcome);
};

#endif // JOBTRACKER_SRC_CVS_CVIMPORTWORKER_HPP

#ifndef JOBTRACKER_SRC_CVS_CVIMPORTEXECUTOR_HPP
#define JOBTRACKER_SRC_CVS_CVIMPORTEXECUTOR_HPP

#include "CvImportWorkerTypes.hpp"

#include <QObject>
#include <QString>
#include <QUrl>

#include <exception>
#include <memory>
#include <variant>

class CvLockWrapper;

class CvImportExecutor final : public QObject
{
public:
	using Outcome = std::variant<CvImportSaveOutcome>;

	CvImportExecutor(QString dataDirectory, CvLockWrapper& cvMutationQueue);
	~CvImportExecutor() override;

	CvImportSaveOutcome process(CvImportRequest request);
	void destroyContext();

private:
	struct PipelineContext;

	PipelineContext& ensureContext();

	static QString fileNameForUrl(const QUrl& sourceUrl);
	static QString importExceptionMessage(
		const std::exception_ptr& exception);
	static CvImportSaveOutcome failureOutcome(
		const CvImportRequest& request,
		QString message);

	QString dataDirectory_;
	CvLockWrapper& cvLock_;
	std::unique_ptr<PipelineContext> context_;
};

#endif // JOBTRACKER_SRC_CVS_CVIMPORTEXECUTOR_HPP

#include "AppBootstrap.hpp"

#include "logging/ApplicationLog.hpp"

#include <QObject>
#include <QWindow>
#include <QQmlContext>
#include <QUrl>

#include <cstdlib>
#include <stdexcept>

namespace {

	QVector<CvDocument> resolveCvDocuments(
		CvRepository& repository,
		CvManagedFileStore& managedFileStore,
		bool& allReadsComplete)
	{
		auto documents = repository.findAll(&allReadsComplete);
		managedFileStore.reconcile(documents);
		return documents;
	}

} // namespace

AppBootstrap::AppBootstrap(QCoreApplication& app)
	: app_{ app }
	, storagePaths_{}
	, database_{ storagePaths_.databasePath() }
	, cvRepository_{ database_.connection() }
	, cvManagedFileStore_{ storagePaths_ }
	, cvFileAccessService_{ storagePaths_ }
	, companyRepository_{ database_.connection() }
	, jobRepository_{ database_.connection() }
	, jobSaveWorker_{ storagePaths_.dataDirectory(), cvLock_ }
	, cvImportWorker_{ storagePaths_.dataDirectory(), cvLock_ }
	, storageMutationGate_{}
	, dataRemovalWorker_{ storagePaths_.dataDirectory() }
	, jobApplicationsController_{
		jobRepository_.findAll(&savedDataComplete_),
		jobSaveWorker_,
		dataRemovalWorker_,
		storageMutationGate_ }
		, cvLibraryController_{
			jobApplicationsController_.jobApplicationListModel(),
			resolveCvDocuments(cvRepository_, cvManagedFileStore_, savedDataComplete_),
			cvRepository_,
			cvFileAccessService_,
			cvImportWorker_,
			dataRemovalWorker_,
			storageMutationGate_ }
			, dashboardController_{ jobApplicationsController_.jobApplicationListModel(), cvLibraryController_.cvListModel() }
	, companyDirectoryController_(
		companyRepository_.findAll(&savedDataComplete_),
		jobApplicationsController_.jobApplicationListModel(),
		contactModel_)
	, contactDirectoryController_(contactModel_)
{
	QObject::connect(
		&app_, &QCoreApplication::aboutToQuit,
		this, &AppBootstrap::beginShutdown);
	// When a job is created or its CV is replaced, the controller publishes the
	// committed CV relationship so the library can refresh its linked-job counts.
	QObject::connect(
		&jobApplicationsController_,
		&JobApplicationsController::cvUsed,
		&cvLibraryController_,
		&CvLibraryController::recordCvUse);
	QObject::connect(
		&jobApplicationsController_,
		&JobApplicationsController::cvReplaced,
		&cvLibraryController_,
		&CvLibraryController::recordCvReplacement);
	QObject::connect(
		&jobApplicationsController_,
		&JobApplicationsController::applicationsDeleted,
		&cvLibraryController_,
		&CvLibraryController::recordApplicationsDeleted);
	/*
	When creating or updating a job, the save service can:
	1. find an existing company
	2. or create a new company.
	Upon successful completion, the controller emits the committed Company value.
	Then, CompanyDirectoryController::publishCompany() adds it when it is not already present.
	*/
	QObject::connect(
		&jobApplicationsController_,
		&JobApplicationsController::companyResolved,
		&companyDirectoryController_,
		&CompanyDirectoryController::publishCompany);
}

AppBootstrap::~AppBootstrap()
{
	// Join persistence work while its gate and controllers still exist. Controller
	// teardown can then release outstanding reservations without exposing a gap.
	storageMutationGate_.beginShutdown();
	const bool jobsClean = jobSaveWorker_.shutdown();
	const bool importsClean = cvImportWorker_.shutdown();
	const bool removalClean = dataRemovalWorker_.shutdown();
	if (normalExit_ && jobsClean && importsClean && removalClean)
		ApplicationLog::record({LogEventType::ControlledShutdownCompleted});
}

int AppBootstrap::run()
{
	connectEngineFailureHandler();
	configureContextProperties();
	loadMainQml();

	const int result = QCoreApplication::exec();
	normalExit_ = result == EXIT_SUCCESS;
	return result;
}

void AppBootstrap::beginShutdown()
{
	if (!shutdownReported_) {
		const auto active = jobApplicationsController_.activeWorkCount() + cvLibraryController_.activeWorkCount();
		const auto pending = jobApplicationsController_.pendingSaveCount() + cvLibraryController_.pendingImportCount()
			+ jobApplicationsController_.pendingDeletionCount() + cvLibraryController_.pendingDeletionCount();
		ApplicationLog::record({LogEventType::ControlledShutdownStarted,
			{LogPendingCounts{.active_ = active, .queued_ = pending - active}}});
		shutdownReported_ = true;
	}
	storageMutationGate_.beginShutdown();
}

void AppBootstrap::recordCloseWait()
{
	ApplicationLog::record({LogEventType::CloseDeferredByUser});
}

void AppBootstrap::recordInterruptAndExit()
{
	ApplicationLog::record({LogEventType::InterruptAndExitSelected,
		{storageMutationGate_.shutdownQueuedCancellationCount(), storageMutationGate_.shutdownActiveCancellationCount()}});
	shutdownReported_ = true;
}

void AppBootstrap::connectEngineFailureHandler()
{   // Exit the application with a failure code if the QML engine cannot create the root object.
	QObject::connect(
		&engine_,
		&QQmlApplicationEngine::objectCreationFailed,
		&app_,
		[]() { QCoreApplication::exit(EXIT_FAILURE); },
		Qt::QueuedConnection);
}

void AppBootstrap::configureContextProperties()
{   // Register application controllers in the QML context so the UI can access them by name.    
	engine_.rootContext()->setContextProperty(QStringLiteral("jobApplicationsController"), &jobApplicationsController_);
	engine_.rootContext()->setContextProperty(QStringLiteral("cvLibraryController"), &cvLibraryController_);
	engine_.rootContext()->setContextProperty(QStringLiteral("dashboardController"), &dashboardController_);
	engine_.rootContext()->setContextProperty(QStringLiteral("companyDirectoryController"), &companyDirectoryController_);
	engine_.rootContext()->setContextProperty(QStringLiteral("contactDirectoryController"), &contactDirectoryController_);
	engine_.rootContext()->setContextProperty(QStringLiteral("storageMutationGate"), &storageMutationGate_);
}

void AppBootstrap::loadMainQml()
{	// Load the main QML entry point and fail startup if the root object was not created.
	engine_.load(QUrl(QStringLiteral("qrc:/JobTracker/qml/Main.qml")));

	const auto roots = engine_.rootObjects();
	auto* const root = roots.isEmpty() ? nullptr : qobject_cast<QWindow*>(roots.constFirst());
	if (!root) {
		ApplicationLog::record({LogEventType::MainWindowCreationFailed,
			{LogFact::InterfaceLoading, LogFact::RootWindowUnavailable}});
		if (roots.isEmpty())
			throw std::runtime_error("Failed to load "
				"qrc:/JobTracker/qml/Main.qml");
		return;
	}
	QObject::connect(root, SIGNAL(closeWaitSelected()), this, SLOT(recordCloseWait()));
	QObject::connect(root, SIGNAL(interruptAndExitSelected()), this, SLOT(recordInterruptAndExit()));
	if (savedDataComplete_)
		ApplicationLog::record({LogEventType::ApplicationStarted});
}

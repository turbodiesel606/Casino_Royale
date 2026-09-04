#include "AppBootstrap.hpp"

#include <QObject>
#include <QDebug>
#include <QQmlContext>
#include <QUrl>

#include <cstdlib>
#include <stdexcept>

namespace {

	QVector<CvDocument> resolveCvDocuments(
		CvRepository& repository,
		CvManagedFileStore& managedFileStore)
	{	

		/*
			EXCEPTION_HANDLING
		*/

		auto documents = repository.findAll();
		const auto recovery = managedFileStore.reconcile(documents);
		if (recovery.removedStagedFileCount_ > 0
			|| recovery.removedDeletionFileCount_ > 0
			|| recovery.restoredDeletionFileCount_ > 0
			|| !recovery.quarantinedFileNames_.isEmpty()) {
			qInfo() << "Managed CV recovery removed"
				<< recovery.removedStagedFileCount_
				<< "staged files, removed"
				<< recovery.removedDeletionFileCount_
				<< "deletion tombstones, restored"
				<< recovery.restoredDeletionFileCount_
				<< "deletion tombstones, and quarantined"
				<< recovery.quarantinedFileNames_.size()
				<< "orphaned files.";
		}
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
	, jobSaveWorker_{ storagePaths_.dataDirectory() }
	, cvImportWorker_{ storagePaths_.dataDirectory() }
	, storageMutationGate_{}
	, dataRemovalWorker_{ storagePaths_.dataDirectory() }
	, jobApplicationsController_{
		jobRepository_.findAll(),
		jobSaveWorker_,
		dataRemovalWorker_,
		storageMutationGate_ }
		, cvLibraryController_{
			jobApplicationsController_.jobApplicationListModel(),
			resolveCvDocuments(cvRepository_, cvManagedFileStore_),
			cvRepository_,
			cvFileAccessService_,
			cvImportWorker_,
			dataRemovalWorker_,
			storageMutationGate_ }
			, dashboardController_{ jobApplicationsController_.jobApplicationListModel(), cvLibraryController_.cvListModel() }
	, companyDirectoryController_(
		companyRepository_.findAll(),
		jobApplicationsController_.jobApplicationListModel(),
		contactModel_)
	, contactDirectoryController_(contactModel_)
{
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

int AppBootstrap::run()
{
	connectEngineFailureHandler();
	configureContextProperties();
	loadMainQml();

	return QCoreApplication::exec();
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
}

void AppBootstrap::loadMainQml()
{	// Load the main QML entry point and fail startup if the root object was not created.
	engine_.load(QUrl(QStringLiteral("qrc:/JobTracker/qml/Main.qml")));

	if (engine_.rootObjects().isEmpty()) {
		throw std::runtime_error("Failed to load "
			"qrc:/JobTracker/qml/Main.qml");
	}
}

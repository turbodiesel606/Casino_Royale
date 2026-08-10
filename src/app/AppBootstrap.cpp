#include "AppBootstrap.hpp"

#include <QObject>
#include <QDebug>
#include <QQmlContext>
#include <QUrl>

#include <cstdlib>
#include <stdexcept>

AppBootstrap::AppBootstrap(QCoreApplication& app)
	: app_{ app }
	, storagePaths_{}
	, database_{ storagePaths_.databasePath() }
	, cvRepository_{ database_.connection() }
	, cvManagedFileStore_{ storagePaths_ }
	, cvFileAccessService_{ storagePaths_ }
	, companyRepository_{ database_.connection() }
	, jobRepository_{ database_.connection() }
	, addJobWorker_{ storagePaths_.dataDirectory() }
	, jobApplicationsController_{ jobRepository_.findAll(), addJobWorker_ }
	, cvLibraryController_{
		jobApplicationsController_.jobApplicationListModel(),
		cvRepository_.findAll(),
		cvRepository_,
		cvFileAccessService_ }
		, dashboardController_{ jobApplicationsController_.jobApplicationListModel(), cvLibraryController_.cvListModel() }
	, companyDirectoryController_(
		companyRepository_.findAll(),
		jobApplicationsController_.jobApplicationListModel(),
		contactModel_)
	, contactDirectoryController_(contactModel_)
{
	const auto recovery = cvManagedFileStore_.reconcile(cvRepository_.findAll());
	if (recovery.removedStagedFileCount_ > 0
		|| !recovery.quarantinedFileNames_.isEmpty()) {
		qInfo() << "Managed CV recovery removed"
			<< recovery.removedStagedFileCount_
			<< "staged files and quarantined"
			<< recovery.quarantinedFileNames_.size()
			<< "orphaned files.";
	}

	// When a new job opening is successfully created, 
	// the JobApplicationsController emits a cvUsed signal, indicating which CV was used.
	QObject::connect(
		&jobApplicationsController_,
		&JobApplicationsController::cvUsed,
		&cvLibraryController_,
		&CvLibraryController::recordCvUse);
	/*
	When adding a job, AddJobService can:
	1. find an existing company 
	2. or create a new company.
	Upon successful completion, the controller emits companyResolved.
	Then, CompanyDirectoryController::publishCompany() adds or updates the company in its model.
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

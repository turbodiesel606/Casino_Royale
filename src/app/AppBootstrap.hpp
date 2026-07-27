#ifndef JOBTRACKER_SRC_APP_APPBOOTSTRAP_HPP
#define JOBTRACKER_SRC_APP_APPBOOTSTRAP_HPP

#include "cvs/CvFileAccessService.hpp"
#include "cvs/CvImportService.hpp"
#include "cvs/CvLibraryController.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "dashboard/DashboardController.hpp"
#include "directory/CompanyDirectoryController.hpp"
#include "directory/CompanyRepository.hpp"
#include "directory/ContactDirectoryController.hpp"
#include "directory/ContactListModel.hpp"
#include "jobs/AddJobService.hpp"
#include "jobs/JobApplicationsController.hpp"
#include "jobs/JobRepository.hpp"
#include "storage/SqliteDatabase.hpp"
#include "storage/StoragePaths.hpp"

#include <QCoreApplication>
#include <QQmlApplicationEngine>


// Creates and wires the application's core services, controllers, and QML engine.
class AppBootstrap final
{
public:
    explicit AppBootstrap(QCoreApplication& app);

    int run();

private:
    void connectEngineFailureHandler();
    void configureContextProperties();
    void loadMainQml();

    QCoreApplication& app_;
    QQmlApplicationEngine engine_;
    StoragePaths storagePaths_; // provides filesystem paths the app uses, especially the database location.
    SQLiteDataBase database_; // opens/manages the SQLite database using the path from StoragePaths.
    CvRepository cvRepository_; // data-access layer for CV records. It talks to the database connection.
    CvManagedFileStore cvManagedFileStore_; // prepares managed files and reconciles interrupted imports.
    CvFileAccessService cvFileAccessService_; // validates and opens CV files from managed storage.
    CompanyRepository companyRepository_; // data-access layer for durable company identities.
    JobRepository jobRepository_; // data-access layer for job application records.
    CvImportService cvImportService_; // coordinates managed CV files with durable CV identities.
    AddJobService addJobService_; // service that handles adding jobs and related persistence/workflow, using DB, job repository, and CV import service.
    JobApplicationsController jobApplicationsController_; // controller exposed to QML for the job applications area. It starts from stored job data and uses AddJobService for operations.
    CvLibraryController cvLibraryController_; // controller for the CV library area. It depends on the job applications model plus CV data.
    DashboardController dashboardController_; // controller for dashboard summaries/aggregates, built from job and CV models.
    ContactListModel contactModel_; // shared contact data model used by directory-related controllers
    CompanyDirectoryController companyDirectoryController_; // controller for company-related UI, using job application data plus the shared contact model.
    ContactDirectoryController contactDirectoryController_; // controller for contact-related UI, using the shared contact model.
};

#endif // JOBTRACKER_SRC_APP_APPBOOTSTRAP_HPP

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

private:

    QCoreApplication& app_;
    QQmlApplicationEngine engine_;
    StoragePaths storagePaths_;
    SqliteDatabase database_; 
    CvRepository cvRepository_; 
    CvManagedFileStore cvManagedFileStore_; 
    CvFileAccessService cvFileAccessService_; 
    CompanyRepository companyRepository_;
    JobRepository jobRepository_;
    CvImportService cvImportService_; 
    AddJobService addJobService_; 
    JobApplicationsController jobApplicationsController_; 
    CvLibraryController cvLibraryController_; 
    DashboardController dashboardController_; 
    ContactListModel contactModel_; 
    CompanyDirectoryController companyDirectoryController_; 
    ContactDirectoryController contactDirectoryController_; 
};

#endif // JOBTRACKER_SRC_APP_APPBOOTSTRAP_HPP

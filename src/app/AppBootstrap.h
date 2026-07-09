#pragma once

#include "cvs/CvImportService.h"
#include "cvs/CvLibraryController.h"
#include "cvs/CvRepository.h"
#include "dashboard/DashboardController.h"
#include "directory/CompanyDirectoryController.h"
#include "directory/ContactDirectoryController.h"
#include "directory/ContactListModel.h"
#include "jobs/AddJobService.h"
#include "jobs/JobApplicationsController.h"
#include "jobs/JobRepository.h"
#include "storage/SqliteDatabase.h"
#include "storage/StoragePaths.h"

#include <QCoreApplication>
#include <QQmlApplicationEngine>

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
    StoragePaths storagePaths_;
    SqliteDatabase database_;
    CvRepository cvRepository_;
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

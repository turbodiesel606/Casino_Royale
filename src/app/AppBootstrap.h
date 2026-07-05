#pragma once

#include "cvs/CvLibraryController.h"
#include "dashboard/DashboardController.h"
#include "directory/CompanyDirectoryController.h"
#include "directory/ContactDirectoryController.h"
#include "directory/ContactListModel.h"
#include "jobs/JobApplicationsController.h"

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
    JobApplicationsController jobApplicationsController_;
    CvLibraryController cvLibraryController_;
    DashboardController dashboardController_;
    ContactListModel contactModel_;
    CompanyDirectoryController companyDirectoryController_;
    ContactDirectoryController contactDirectoryController_;
    QQmlApplicationEngine engine_;
};

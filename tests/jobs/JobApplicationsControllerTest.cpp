#include "jobs/JobApplicationsController.hpp"

#include "../support/JobApplicationTestData.hpp"

#include <QSignalSpy>
#include <QtTest/QtTest>

namespace {

int roleForName(const QAbstractItemModel& model, const QByteArray& roleName)
{
    const auto names = model.roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        if (it.value() == roleName) {
            return it.key();
        }
    }
    return -1;
}

}

class JobApplicationsControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void modelExposesNamedRoles();
    void modelStartsEmpty();
    void modelExposesExplicitApplications();
    void controllerExposesSelectedApplication();
    void controllerIgnoresInvalidSelection();
    void controllerFiltersBySearchTextAndStatus();
    void controllerValidatesSelectedApplication();
};

void JobApplicationsControllerTest::modelExposesNamedRoles()
{
    JobApplicationsController controller;
    const auto* model = controller.applicationsModel();

    QVERIFY(roleForName(*model, "id") > 0);
    QVERIFY(roleForName(*model, "companyName") > 0);
    QVERIFY(roleForName(*model, "jobTitle") > 0);
    QVERIFY(roleForName(*model, "cvFileName") > 0);
    QVERIFY(roleForName(*model, "statusLabel") > 0);
    QVERIFY(roleForName(*model, "statusAccent") > 0);
    QVERIFY(roleForName(*model, "nextStep") > 0);
    QVERIFY(roleForName(*model, "notes") > 0);
}

void JobApplicationsControllerTest::modelStartsEmpty()
{
    JobApplicationsController controller;
    const auto* model = controller.applicationsModel();

    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(controller.applicationCount(), 0);
    QCOMPARE(controller.selectedApplicationIndex(), -1);
    QVERIFY(controller.selectedApplicationId().isEmpty());
    QVERIFY(controller.selectedApplication().isEmpty());
    QCOMPARE(controller.resultSummary(), QStringLiteral("Showing 0 applications"));
}

void JobApplicationsControllerTest::modelExposesExplicitApplications()
{
    JobApplicationsController controller(testsupport::makeJobApplications());
    const auto* model = controller.applicationsModel();
    const auto firstRow = model->index(0, 0);

    QCOMPARE(model->rowCount(), 6);
    QCOMPARE(model->data(firstRow, roleForName(*model, "companyName")).toString(), QStringLiteral("KDAB"));
    QCOMPARE(model->data(firstRow, roleForName(*model, "jobTitle")).toString(), QStringLiteral("C++/Qt Developer"));
    QCOMPARE(model->data(firstRow, roleForName(*model, "cvFileName")).toString(), QStringLiteral("CV_Qt_2026.pdf"));
    QCOMPARE(model->data(firstRow, roleForName(*model, "statusLabel")).toString(), QStringLiteral("Applied"));
}

void JobApplicationsControllerTest::controllerExposesSelectedApplication()
{
    JobApplicationsController controller(testsupport::makeJobApplications());
    QSignalSpy selectedSpy(&controller, &JobApplicationsController::selectedApplicationChanged);

    controller.selectApplication(1);

    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(controller.selectedApplicationIndex(), 1);
    QCOMPARE(controller.selectedApplicationId(), QStringLiteral("job-techsoft-qt-qml"));
    QCOMPARE(controller.selectedApplication().value(QStringLiteral("companyName")).toString(), QStringLiteral("TechSoft"));
    QCOMPARE(controller.selectedApplication().value(QStringLiteral("statusLabel")).toString(), QStringLiteral("Interview"));
}

void JobApplicationsControllerTest::controllerIgnoresInvalidSelection()
{
    JobApplicationsController controller;
    QSignalSpy selectedSpy(&controller, &JobApplicationsController::selectedApplicationChanged);

    controller.selectApplication(-1);
    controller.selectApplication(100);

    QCOMPARE(selectedSpy.count(), 0);
    QCOMPARE(controller.selectedApplicationIndex(), -1);
    QVERIFY(controller.selectedApplicationId().isEmpty());
}

void JobApplicationsControllerTest::controllerFiltersBySearchTextAndStatus()
{
    JobApplicationsController controller(testsupport::makeJobApplications());
    QSignalSpy filtersSpy(&controller, &JobApplicationsController::filtersChanged);

    controller.setSearchText(QStringLiteral("TechSoft"));

    QCOMPARE(filtersSpy.count(), 1);
    QCOMPARE(controller.applicationCount(), 1);
    QCOMPARE(controller.selectedApplicationIndex(), 0);
    QCOMPARE(controller.selectedApplicationId(), QStringLiteral("job-techsoft-qt-qml"));
    QCOMPARE(controller.resultSummary(), QStringLiteral("Showing 1 to 1 of 1 applications"));

    controller.clearFilters();
    controller.setStatusFilter(QStringLiteral("Interview"));

    QCOMPARE(controller.applicationCount(), 2);
    QCOMPARE(controller.selectedApplication().value(QStringLiteral("statusLabel")).toString(), QStringLiteral("Interview"));

    controller.clearFilters();
    QCOMPARE(controller.applicationCount(), 6);
}

void JobApplicationsControllerTest::controllerValidatesSelectedApplication()
{
    JobApplicationsController controller(testsupport::makeJobApplications());

    QVERIFY(controller.validateSelectedApplication().isEmpty());
}

QTEST_APPLESS_MAIN(JobApplicationsControllerTest)

#include "JobApplicationsControllerTest.moc"

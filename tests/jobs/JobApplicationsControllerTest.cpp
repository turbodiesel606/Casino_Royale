#include "jobs/JobApplicationsController.hpp"

#include "../support/AddJobTestFixture.hpp"
#include "../support/JobApplicationTestData.hpp"

#include <QSignalSpy>
#include <QElapsedTimer>
#include <QtTest/QtTest>

#include <utility>

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
    void initTestCase();
    void modelExposesNamedRoles();
    void modelStartsEmpty();
    void modelExposesExplicitApplications();
    void controllerExposesSelectedApplication();
    void controllerIgnoresInvalidSelection();
    void controllerFiltersBySearchTextAndStatus();
    void selectionPublishesControllerContracts();
    void controllerValidatesSelectedApplication();
    void controllerQueuesRawAdmissionsBeforeWorkerValidation();

private:
    testsupport::AddJobWorkerTestFixture fixture_;
};

void JobApplicationsControllerTest::initTestCase()
{
    QVERIFY(fixture_.isValid());
}

void JobApplicationsControllerTest::modelExposesNamedRoles()
{
    JobApplicationsController controller{{}, fixture_.worker_};
    const auto* model = controller.applicationsModel();

    QVERIFY(roleForName(*model, "id") > 0);
    QVERIFY(roleForName(*model, "companyName") > 0);
    QVERIFY(roleForName(*model, "jobTitle") > 0);
    QVERIFY(roleForName(*model, "cvFileName") > 0);
    QVERIFY(roleForName(*model, "statusLabel") > 0);
    QVERIFY(roleForName(*model, "statusAccent") > 0);
    QVERIFY(roleForName(*model, "nextStep") > 0);
    QVERIFY(roleForName(*model, "notes") > 0);
    QVERIFY(roleForName(*model, "statusValue") > 0);
    QVERIFY(roleForName(*model, "workFormatValue") > 0);
    QVERIFY(roleForName(*model, "appliedDateValue") > 0);
    QVERIFY(roleForName(*model, "createdAt") > 0);
    QVERIFY(roleForName(*model, "updatedAt") > 0);
}

void JobApplicationsControllerTest::modelStartsEmpty()
{
    JobApplicationsController controller{{}, fixture_.worker_};
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
    JobApplicationsController controller{testsupport::makeJobApplications(), fixture_.worker_};
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
    JobApplicationsController controller{testsupport::makeJobApplications(), fixture_.worker_};
    QSignalSpy selectedIndexSpy(&controller, &JobApplicationsController::selectedApplicationIndexChanged);
    QSignalSpy selectedIdSpy(&controller, &JobApplicationsController::selectedApplicationIdChanged);
    QSignalSpy selectedDataSpy(&controller, &JobApplicationsController::selectedApplicationChanged);

    controller.selectApplication(1);

    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedIdSpy.count(), 1);
    QCOMPARE(selectedDataSpy.count(), 1);
    QCOMPARE(controller.selectedApplicationIndex(), 1);
    QCOMPARE(controller.selectedApplicationId(), QStringLiteral("job-techsoft-qt-qml"));
    QCOMPARE(controller.selectedApplication().value(QStringLiteral("companyName")).toString(), QStringLiteral("TechSoft"));
    QCOMPARE(controller.selectedApplication().value(QStringLiteral("statusLabel")).toString(), QStringLiteral("Interview"));
}

void JobApplicationsControllerTest::controllerIgnoresInvalidSelection()
{
    JobApplicationsController controller{{}, fixture_.worker_};
    QSignalSpy selectedIndexSpy(&controller, &JobApplicationsController::selectedApplicationIndexChanged);
    QSignalSpy selectedIdSpy(&controller, &JobApplicationsController::selectedApplicationIdChanged);
    QSignalSpy selectedDataSpy(&controller, &JobApplicationsController::selectedApplicationChanged);

    controller.selectApplication(-1);
    controller.selectApplication(100);

    QCOMPARE(selectedIndexSpy.count(), 0);
    QCOMPARE(selectedIdSpy.count(), 0);
    QCOMPARE(selectedDataSpy.count(), 0);
    QCOMPARE(controller.selectedApplicationIndex(), -1);
    QVERIFY(controller.selectedApplicationId().isEmpty());
}

void JobApplicationsControllerTest::controllerFiltersBySearchTextAndStatus()
{
    JobApplicationsController controller{testsupport::makeJobApplications(), fixture_.worker_};
    QSignalSpy searchTextSpy(&controller, &JobApplicationsController::searchTextChanged);
    QSignalSpy statusFilterSpy(&controller, &JobApplicationsController::statusFilterChanged);
    QSignalSpy countSpy(&controller, &JobApplicationsController::applicationCountChanged);
    QSignalSpy resultSummarySpy(&controller, &JobApplicationsController::resultSummaryChanged);

    controller.setSearchText(QStringLiteral("TechSoft"));

    QCOMPARE(searchTextSpy.count(), 1);
    QCOMPARE(statusFilterSpy.count(), 0);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(resultSummarySpy.count(), 1);
    QCOMPARE(controller.applicationCount(), 1);
    QCOMPARE(controller.selectedApplicationIndex(), 0);
    QCOMPARE(controller.selectedApplicationId(), QStringLiteral("job-techsoft-qt-qml"));
    QCOMPARE(controller.resultSummary(), QStringLiteral("Showing 1 to 1 of 1 applications"));

    controller.clearFilters();
    controller.setStatusFilter(QStringLiteral("Interview"));

    QCOMPARE(searchTextSpy.count(), 2);
    QCOMPARE(statusFilterSpy.count(), 1);
    QCOMPARE(countSpy.count(), 3);
    QCOMPARE(resultSummarySpy.count(), 3);
    QCOMPARE(controller.applicationCount(), 2);
    QCOMPARE(controller.selectedApplication().value(QStringLiteral("statusLabel")).toString(), QStringLiteral("Interview"));

    controller.clearFilters();
    QCOMPARE(controller.applicationCount(), 6);
    QCOMPARE(statusFilterSpy.count(), 2);
    QCOMPARE(countSpy.count(), 4);
    QCOMPARE(resultSummarySpy.count(), 4);

    controller.clearFilters();
    QCOMPARE(searchTextSpy.count(), 2);
    QCOMPARE(statusFilterSpy.count(), 2);
    QCOMPARE(countSpy.count(), 4);
}

void JobApplicationsControllerTest::selectionPublishesControllerContracts()
{
    JobApplicationsController controller{testsupport::makeJobApplications(), fixture_.worker_};
    controller.selectApplication(1);
    QCOMPARE(controller.selectedApplicationId(), QStringLiteral("job-techsoft-qt-qml"));

    QSignalSpy selectedIndexSpy{&controller, &JobApplicationsController::selectedApplicationIndexChanged};
    QSignalSpy selectedIdSpy{&controller, &JobApplicationsController::selectedApplicationIdChanged};
    QSignalSpy selectedDataSpy{&controller, &JobApplicationsController::selectedApplicationChanged};
    auto insertedApplication = testsupport::makeJobApplication(
        QStringLiteral("job-newest"),
        QStringLiteral("company-newest"),
        QStringLiteral("Newest Company"),
        QStringLiteral("Newest Role"),
        QStringLiteral("cv-newest"),
        QStringLiteral("CV_Newest.pdf"),
        QDate{2026, 5, 20},
        JobStatus::Offer,
        QStringLiteral("Decision"));

    controller.jobApplicationListModel().appendApplication(std::move(insertedApplication));

    QCOMPARE(controller.selectedApplicationId(), QStringLiteral("job-techsoft-qt-qml"));
    QCOMPARE(controller.selectedApplicationIndex(), 2);
    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedIdSpy.count(), 0);
    QCOMPARE(selectedDataSpy.count(), 0);

    selectedIndexSpy.clear();
    selectedIdSpy.clear();
    selectedDataSpy.clear();
    controller.setStatusFilter(QStringLiteral("Applied"));
    QCOMPARE(controller.selectedApplicationId(), QStringLiteral("job-kdab-cpp-qt"));
    QCOMPARE(controller.selectedApplicationIndex(), 0);
    QCOMPARE(selectedIdSpy.count(), 1);
    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedDataSpy.count(), 1);
}

void JobApplicationsControllerTest::controllerValidatesSelectedApplication()
{
    auto applications = testsupport::makeJobApplications();
    applications.first().jobUrl_ = {};
    JobApplicationsController controller{applications, fixture_.worker_};

    QVERIFY(controller.validateSelectedApplication().isEmpty());

    applications.first().jobUrl_ = QUrl{QStringLiteral("https:job-posting"), QUrl::StrictMode};
    JobApplicationsController invalidController{std::move(applications), fixture_.worker_};
    QVERIFY(!invalidController.validateSelectedApplication().isEmpty());
}

void JobApplicationsControllerTest::controllerQueuesRawAdmissionsBeforeWorkerValidation()
{
    JobApplicationsController controller{{}, fixture_.worker_};
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationQueued};
    QSignalSpy acceptedSpy{&controller, &JobApplicationsController::applicationAccepted};
    QSignalSpy rejectedSpy{&controller, &JobApplicationsController::applicationRejected};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};

    QElapsedTimer callTimer;
    callTimer.start();
    controller.createApplication(
        {
            {QStringLiteral("jobTitle"), QStringLiteral(" ")},
            {QStringLiteral("jobUrl"), QStringLiteral("ftp://example.com/job")},
            {QStringLiteral("companyName"), QStringLiteral(" ")},
            {QStringLiteral("workFormat"), QStringLiteral("Office")},
            {QStringLiteral("status"), QStringLiteral("Pending")},
            {QStringLiteral("appliedDate"), QStringLiteral("2026-99-87")},
        },
        {});

    QVERIFY2(callTimer.elapsed() < 1000, "Raw FIFO admission blocked the controller thread.");
    QCOMPARE(queuedSpy.count(), 1);
    QCOMPARE(queuedSpy.first().at(0).toULongLong(), quint64(1));
    QCOMPARE(rejectedSpy.count(), 0);
    QCOMPARE(controller.pendingSaveCount(), 1);
    QVERIFY(controller.saving());

    QTRY_COMPARE(rejectedSpy.count(), 1);
    QCOMPARE(rejectedSpy.first().at(0).toULongLong(), quint64(1));
    const auto fullErrors = rejectedSpy.first().at(1).toMap();
    const QStringList expectedFields{
        QStringLiteral("jobTitle"),
        QStringLiteral("jobUrl"),
        QStringLiteral("companyName"),
        QStringLiteral("workFormat"),
        QStringLiteral("status"),
        QStringLiteral("appliedDate"),
        QStringLiteral("cv"),
    };
    for (const auto& field : expectedFields) {
        QVERIFY2(fullErrors.contains(field), qPrintable(field));
    }
    QVERIFY(!rejectedSpy.first().at(2).toString().isEmpty());
    QCOMPARE(acceptedSpy.count(), 0);
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(controller.pendingSaveCount(), 0);
    QVERIFY(!controller.saving());

    const auto sourcePath = fixture_.storage_.createFile(QStringLiteral("accepted.pdf"));
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("  Accepted Role  ")),
        QUrl::fromLocalFile(sourcePath));

    QCOMPARE(queuedSpy.count(), 2);
    QCOMPARE(queuedSpy.at(1).at(0).toULongLong(), quint64(2));
    QTRY_COMPARE(completedSpy.count(), 1);
    QCOMPARE(acceptedSpy.count(), 1);
    QCOMPARE(acceptedSpy.first().at(0).toULongLong(), quint64(2));
    QCOMPARE(acceptedSpy.first().at(1).toString(), QStringLiteral("Accepted Role"));
    QVERIFY(completedSpy.first().at(2).toBool());
    QCOMPARE(controller.pendingSaveCount(), 0);
}

QTEST_GUILESS_MAIN(JobApplicationsControllerTest)

#include "JobApplicationsControllerTest.moc"

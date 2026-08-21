#include "jobs/JobApplicationsController.hpp"
#include "maintenance/DataRemovalWorker.hpp"
#include "maintenance/StorageMutationGate.hpp"

#include "../support/AddJobTestFixture.hpp"
#include "../support/JobApplicationTestData.hpp"

#include <QSignalSpy>
#include <QSqlQuery>
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
    void bulkSelectionUsesVisibleRowsAndClearsOnFilters();
    void selectionPublishesControllerContracts();
    void controllerValidatesSelectedApplication();
    void controllerValidatesBeforeQueueAdmission();
    void controllerRejectsInvalidUpdateBeforeQueueAdmission();
    void controllerPublishesSuccessfulUpdateInPlace();
    void controllerRetainsModelWhenUpdateFails();
    void createAndUpdateShareOneFifo();
    void asyncDeletionRemovesModelAndPreservesCvAndCompany();
    void mutationGateRejectsAddAndDeletionAdmission();

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

void JobApplicationsControllerTest::bulkSelectionUsesVisibleRowsAndClearsOnFilters()
{
    JobApplicationsController controller{testsupport::makeJobApplications(), fixture_.worker_};
    QSignalSpy checkedSpy{&controller, &JobApplicationsController::checkedApplicationsChanged};

    controller.toggleApplicationChecked(0);
    QCOMPARE(controller.checkedApplicationCount(), 1);
    QVERIFY(controller.someVisibleApplicationsChecked());
    QVERIFY(!controller.allVisibleApplicationsChecked());

    controller.setAllVisibleApplicationsChecked(true);
    QCOMPARE(controller.checkedApplicationCount(), controller.applicationCount());
    QVERIFY(controller.allVisibleApplicationsChecked());

    controller.setSearchText(QStringLiteral("KDAB"));
    QCOMPARE(controller.checkedApplicationCount(), 0);
    QVERIFY(!controller.allVisibleApplicationsChecked());
    controller.setAllVisibleApplicationsChecked(true);
    QCOMPARE(controller.checkedApplicationCount(), 1);

    controller.setStatusFilter(QStringLiteral("Interview"));
    QCOMPARE(controller.checkedApplicationCount(), 0);
    QVERIFY(checkedSpy.count() >= 4);
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

void JobApplicationsControllerTest::controllerValidatesBeforeQueueAdmission()
{
    JobApplicationsController controller{{}, fixture_.worker_};
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationQueued};
    QSignalSpy failedSpy{&controller, &JobApplicationsController::saveFailed};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy pendingCountSpy{&controller, &JobApplicationsController::pendingSaveCountChanged};
    QSignalSpy savingSpy{&controller, &JobApplicationsController::savingChanged};

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

    QCOMPARE(failedSpy.count(), 1);
    const auto fullErrors = failedSpy.first().at(0).toMap();
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
    QVERIFY(!failedSpy.first().at(1).toString().isEmpty());
    QCOMPARE(queuedSpy.count(), 0);
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(pendingCountSpy.count(), 0);
    QCOMPARE(savingSpy.count(), 0);
    QCOMPARE(controller.pendingSaveCount(), 0);
    QVERIFY(!controller.saving());
    QVERIFY(!fixture_.worker_.isRunning());

    const auto sourcePath = fixture_.storage_.createFile(QStringLiteral("accepted.pdf"));
    auto validValues = testsupport::validJobFormValues(QStringLiteral("  Accepted Role  "));
    validValues.insert(QStringLiteral("jobUrl"), QStringLiteral("  https://example.com/jobs/accepted  "));
    validValues.insert(QStringLiteral("companyName"), QStringLiteral("  Example Company  "));
    validValues.insert(QStringLiteral("workFormat"), QStringLiteral("  hybrid  "));
    validValues.insert(QStringLiteral("city"), QStringLiteral("  Baku  "));
    validValues.insert(QStringLiteral("salary"), QStringLiteral("  5000  "));
    validValues.insert(QStringLiteral("status"), QStringLiteral("  interview  "));
    validValues.insert(QStringLiteral("appliedDate"), QStringLiteral("  2026-08-05  "));
    validValues.insert(QStringLiteral("nextStep"), QStringLiteral("  Technical interview  "));
    validValues.insert(QStringLiteral("description"), QStringLiteral("  Build Qt applications  "));
    validValues.insert(QStringLiteral("requirements"), QStringLiteral("  Modern C++  "));
    validValues.insert(QStringLiteral("techStack"), QStringLiteral(" Qt, C++ , qt, QML "));
    validValues.insert(QStringLiteral("notes"), QStringLiteral("  Follow up Friday  "));
    controller.createApplication(
        validValues,
        QUrl::fromLocalFile(sourcePath));
    validValues.clear();

    QCOMPARE(queuedSpy.count(), 1);
    QCOMPARE(queuedSpy.first().at(0).toULongLong(), quint64(1));
    QTRY_COMPARE(completedSpy.count(), 1);
    QCOMPARE(completedSpy.first().at(0).toULongLong(), quint64(1));
    QCOMPARE(completedSpy.first().at(1).toString(), QStringLiteral("Accepted Role"));
    QVERIFY(completedSpy.first().at(2).toBool());
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(pendingCountSpy.count(), 2);
    QCOMPARE(savingSpy.count(), 2);
    QCOMPARE(controller.pendingSaveCount(), 0);

    const auto storedApplications = fixture_.jobRepository_.findAll();
    QCOMPARE(storedApplications.size(), 1);
    const auto& application = storedApplications.first();
    QCOMPARE(application.jobTitle_, QStringLiteral("Accepted Role"));
    QCOMPARE(application.jobUrl_, QUrl{QStringLiteral("https://example.com/jobs/accepted")});
    QCOMPARE(application.companyName_, QStringLiteral("Example Company"));
    QCOMPARE(application.workFormat_, WorkFormat::Hybrid);
    QCOMPARE(application.city_, QStringLiteral("Baku"));
    QCOMPARE(application.salary_, QStringLiteral("5000"));
    QCOMPARE(application.status_, JobStatus::Interview);
    QCOMPARE(application.appliedDate_, QDate(2026, 8, 5));
    QCOMPARE(application.nextStep_, QStringLiteral("Technical interview"));
    QCOMPARE(application.description_, QStringLiteral("Build Qt applications"));
    QCOMPARE(application.requirements_, QStringLiteral("Modern C++"));
    QCOMPARE(
        application.techStack_,
        QStringList({
            QStringLiteral("Qt"),
            QStringLiteral("C++"),
            QStringLiteral("QML")}));
    QCOMPARE(application.notes_, QStringLiteral("Follow up Friday"));
}

void JobApplicationsControllerTest::controllerRejectsInvalidUpdateBeforeQueueAdmission()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto created = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("update-invalid.pdf"))));
    QVERIFY(created.success_);
    JobApplicationsController controller{fixture.jobRepository_.findAll(), fixture.worker_};
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationUpdateQueued};
    QSignalSpy rejectedSpy{&controller, &JobApplicationsController::applicationUpdateRejected};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationUpdateCompleted};
    QSignalSpy pendingSpy{&controller, &JobApplicationsController::pendingSaveCountChanged};

    auto invalidValues = testsupport::validJobFormValues();
    invalidValues.insert(QStringLiteral("jobTitle"), QStringLiteral(" "));
    invalidValues.insert(QStringLiteral("companyName"), QStringLiteral(" "));
    invalidValues.insert(QStringLiteral("status"), QStringLiteral("Pending"));
    invalidValues.insert(QStringLiteral("appliedDate"), QStringLiteral("bad-date"));
    controller.updateApplication(created.application_.id_, invalidValues, {});

    QCOMPARE(rejectedSpy.count(), 1);
    QCOMPARE(rejectedSpy.first().at(0).toULongLong(), quint64(0));
    QCOMPARE(rejectedSpy.first().at(1).toString(), created.application_.id_);
    const auto errors = rejectedSpy.first().at(2).toMap();
    QVERIFY(errors.contains(QStringLiteral("jobTitle")));
    QVERIFY(errors.contains(QStringLiteral("companyName")));
    QVERIFY(errors.contains(QStringLiteral("status")));
    QVERIFY(errors.contains(QStringLiteral("appliedDate")));
    QCOMPARE(queuedSpy.count(), 0);
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(pendingSpy.count(), 0);
    QCOMPARE(controller.pendingSaveCount(), 0);
    QVERIFY(!controller.updatingApplication());
    QVERIFY(!fixture.worker_.isRunning());
    QCOMPARE(
        fixture.jobRepository_.findById(created.application_.id_)->jobTitle_,
        created.application_.jobTitle_);
}

void JobApplicationsControllerTest::controllerPublishesSuccessfulUpdateInPlace()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto created = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(fixture.storage_.createFile(
            QStringLiteral("update-original.pdf"),
            QByteArrayLiteral("%PDF original controller CV"))));
    QVERIFY(created.success_);
    JobApplicationsController controller{fixture.jobRepository_.findAll(), fixture.worker_};
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationUpdateQueued};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationUpdateCompleted};
    QSignalSpy updatingSpy{&controller, &JobApplicationsController::updatingApplicationChanged};
    QSignalSpy selectedSpy{&controller, &JobApplicationsController::selectedApplicationChanged};
    QSignalSpy dataChangedSpy{
        &controller.jobApplicationListModel(),
        &QAbstractItemModel::dataChanged};
    QSignalSpy companySpy{&controller, &JobApplicationsController::companyResolved};
    int cvReplacementCount = 0;
    QString previousCvId;
    connect(
        &controller,
        &JobApplicationsController::cvReplaced,
        this,
        [&cvReplacementCount, &previousCvId](
            const QString& previousId,
            const CvDocument&,
            const QString&,
            CvImportDisposition) {
            ++cvReplacementCount;
            previousCvId = previousId;
        });

    auto values = testsupport::validJobFormValues(QStringLiteral("Updated Controller Role"));
    values.insert(QStringLiteral("companyName"), QStringLiteral("Updated Company"));
    values.insert(QStringLiteral("status"), QStringLiteral("Interview"));
    values.insert(QStringLiteral("techStack"), QStringLiteral("Qt, C++, QML"));
    const auto replacementUrl = QUrl::fromLocalFile(fixture.storage_.createFile(
        QStringLiteral("update-replacement.pdf"),
        QByteArrayLiteral("%PDF replacement controller CV")));

    controller.updateApplication(created.application_.id_, values, replacementUrl);

    QCOMPARE(queuedSpy.count(), 1);
    QVERIFY(controller.updatingApplication());
    QTRY_COMPARE_WITH_TIMEOUT(completedSpy.count(), 1, 10000);
    QCOMPARE(completedSpy.first().at(0).toULongLong(), quint64(1));
    QCOMPARE(completedSpy.first().at(1).toString(), created.application_.id_);
    QCOMPARE(completedSpy.first().at(2).toString(), QStringLiteral("Updated Controller Role"));
    QVERIFY(completedSpy.first().at(3).toBool());
    QVERIFY(completedSpy.first().at(4).toMap().isEmpty());
    QCOMPARE(updatingSpy.count(), 2);
    QVERIFY(!controller.updatingApplication());
    QCOMPARE(controller.pendingSaveCount(), 0);
    QCOMPARE(dataChangedSpy.count(), 1);
    const auto changedRoles = dataChangedSpy.first().at(2).value<QList<int>>();
    QVERIFY(changedRoles.contains(JobApplicationListModel::JobTitleRole));
    QVERIFY(changedRoles.contains(JobApplicationListModel::CompanyIdRole));
    QVERIFY(changedRoles.contains(JobApplicationListModel::StatusValueRole));
    QVERIFY(changedRoles.contains(JobApplicationListModel::CvIdRole));
    QVERIFY(changedRoles.contains(JobApplicationListModel::TechStackRole));
    QVERIFY(changedRoles.contains(JobApplicationListModel::UpdatedAtRole));
    QVERIFY(!changedRoles.contains(JobApplicationListModel::CreatedAtRole));
    QVERIFY(selectedSpy.count() >= 1);
    QCOMPARE(companySpy.count(), 1);
    QCOMPARE(cvReplacementCount, 1);
    QCOMPARE(previousCvId, created.cvDocument_.id_);

    const auto selected = controller.selectedApplication();
    QCOMPARE(selected.value(QStringLiteral("id")).toString(), created.application_.id_);
    QCOMPARE(selected.value(QStringLiteral("jobTitle")).toString(), QStringLiteral("Updated Controller Role"));
    QCOMPARE(selected.value(QStringLiteral("companyName")).toString(), QStringLiteral("Updated Company"));
    QCOMPARE(selected.value(QStringLiteral("statusLabel")).toString(), QStringLiteral("Interview"));
    QCOMPARE(selected.value(QStringLiteral("techStack")).toStringList(), QStringList({
        QStringLiteral("Qt"),
        QStringLiteral("C++"),
        QStringLiteral("QML")}));
}

void JobApplicationsControllerTest::controllerRetainsModelWhenUpdateFails()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto created = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("update-failure.pdf"))));
    QVERIFY(created.success_);
    QSqlQuery trigger{fixture.database_.connection()};
    QVERIFY(trigger.exec(QStringLiteral(
        "CREATE TRIGGER reject_controller_update BEFORE UPDATE ON jobs "
        "BEGIN SELECT RAISE(FAIL, 'forced controller update failure'); END")));
    JobApplicationsController controller{fixture.jobRepository_.findAll(), fixture.worker_};
    const auto before = controller.selectedApplication();
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationUpdateCompleted};
    QSignalSpy dataChangedSpy{
        &controller.jobApplicationListModel(),
        &QAbstractItemModel::dataChanged};

    auto values = testsupport::validJobFormValues(QStringLiteral("Rejected Update"));
    values.insert(QStringLiteral("companyName"), QStringLiteral("Example Company"));
    controller.updateApplication(created.application_.id_, values, {});

    QTRY_COMPARE_WITH_TIMEOUT(completedSpy.count(), 1, 10000);
    QVERIFY(!completedSpy.first().at(3).toBool());
    QCOMPARE(dataChangedSpy.count(), 0);
    QCOMPARE(controller.selectedApplication(), before);
    QVERIFY(!controller.updatingApplication());
    const auto stored = fixture.jobRepository_.findById(created.application_.id_);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->jobTitle_, created.application_.jobTitle_);
}

void JobApplicationsControllerTest::createAndUpdateShareOneFifo()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto existing = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("fifo-existing.pdf"))));
    QVERIFY(existing.success_);
    JobApplicationsController controller{fixture.jobRepository_.findAll(), fixture.worker_};
    QSignalSpy createQueuedSpy{&controller, &JobApplicationsController::applicationQueued};
    QSignalSpy updateQueuedSpy{&controller, &JobApplicationsController::applicationUpdateQueued};
    QSignalSpy createCompletedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy updateCompletedSpy{&controller, &JobApplicationsController::applicationUpdateCompleted};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    QStringList completionOrder;
    connect(
        &controller,
        &JobApplicationsController::applicationSaveCompleted,
        this,
        [&completionOrder]() { completionOrder.append(QStringLiteral("create")); });
    connect(
        &controller,
        &JobApplicationsController::applicationUpdateCompleted,
        this,
        [&completionOrder]() { completionOrder.append(QStringLiteral("update")); });

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("FIFO Created Role")),
        QUrl::fromLocalFile(fixture.storage_.createFile(
            QStringLiteral("fifo-created.pdf"),
            QByteArrayLiteral("%PDF fifo created"))));
    auto updateValues = testsupport::validJobFormValues(QStringLiteral("FIFO Updated Role"));
    updateValues.insert(QStringLiteral("companyName"), QStringLiteral("Example Company"));
    controller.updateApplication(existing.application_.id_, updateValues, {});

    QCOMPARE(createQueuedSpy.count(), 1);
    QCOMPARE(updateQueuedSpy.count(), 1);
    QCOMPARE(createQueuedSpy.first().at(0).toULongLong(), quint64(1));
    QCOMPARE(updateQueuedSpy.first().at(0).toULongLong(), quint64(2));
    QCOMPARE(controller.pendingSaveCount(), 2);
    QVERIFY(controller.updatingApplication());

    QTRY_COMPARE_WITH_TIMEOUT(updateCompletedSpy.count(), 1, 10000);
    QCOMPARE(createCompletedSpy.count(), 1);
    QCOMPARE(completionOrder, QStringList({
        QStringLiteral("create"),
        QStringLiteral("update")}));
    QCOMPARE(drainedSpy.count(), 1);
    QCOMPARE(controller.pendingSaveCount(), 0);
    QVERIFY(!controller.saving());
    QVERIFY(!controller.updatingApplication());
    QCOMPARE(fixture.jobRepository_.findAll().size(), 2);
    QCOMPARE(
        fixture.jobRepository_.findById(existing.application_.id_)->jobTitle_,
        QStringLiteral("FIFO Updated Role"));
}

void JobApplicationsControllerTest::asyncDeletionRemovesModelAndPreservesCvAndCompany()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    DataRemovalWorker removalWorker{fixture.storage_.paths().dataDirectory()};
    StorageMutationGate mutationGate;
    JobApplicationsController controller{
        {},
        fixture.worker_,
        removalWorker,
        mutationGate};
    QSignalSpy savedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy deletedSpy{&controller, &JobApplicationsController::applicationDeletionCompleted};
    QSignalSpy rowsRemovedSpy{
        controller.applicationsModel(),
        &QAbstractItemModel::rowsRemoved};

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Delete Me")),
        QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("delete-me.pdf"))));
    QTRY_COMPARE_WITH_TIMEOUT(savedSpy.count(), 1, 10000);
    QVERIFY(savedSpy.first().at(2).toBool());
    QCOMPARE(controller.applicationCount(), 1);

    controller.setAllVisibleApplicationsChecked(true);
    QVERIFY(controller.canDeleteApplications());
    controller.deleteCheckedApplications();
    QVERIFY(controller.deletingApplications());
    QCOMPARE(controller.pendingDeletionCount(), 1);

    QTRY_COMPARE_WITH_TIMEOUT(deletedSpy.count(), 1, 10000);
    QCOMPARE(deletedSpy.first().at(0).toInt(), 1);
    QCOMPARE(deletedSpy.first().at(1).toInt(), 0);
    QCOMPARE(controller.applicationCount(), 0);
    QCOMPARE(controller.checkedApplicationCount(), 0);
    QCOMPARE(rowsRemovedSpy.count(), 1);
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QCOMPARE(fixture.cvRepository_.findAll().size(), 1);
    QCOMPARE(fixture.companyRepository_.findAll().size(), 1);
    QVERIFY(!mutationGate.removalActive());
}

void JobApplicationsControllerTest::mutationGateRejectsAddAndDeletionAdmission()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    DataRemovalWorker removalWorker{fixture.storage_.paths().dataDirectory()};
    StorageMutationGate mutationGate;
    JobApplicationsController controller{
        testsupport::makeJobApplications(),
        fixture.worker_,
        removalWorker,
        mutationGate};
    QSignalSpy failedSpy{&controller, &JobApplicationsController::saveFailed};
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationQueued};
    QSignalSpy updateRejectedSpy{&controller, &JobApplicationsController::applicationUpdateRejected};
    QSignalSpy updateQueuedSpy{&controller, &JobApplicationsController::applicationUpdateQueued};
    QSignalSpy deletionSpy{&controller, &JobApplicationsController::applicationDeletionCompleted};

    QVERIFY(mutationGate.beginRemoval());
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Blocked Add")),
        QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("blocked.pdf"))));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(queuedSpy.count(), 0);
    QCOMPARE(controller.pendingSaveCount(), 0);
    auto updateValues = testsupport::validJobFormValues(QStringLiteral("Blocked Update"));
    updateValues.insert(QStringLiteral("companyName"), QStringLiteral("KDAB"));
    controller.updateApplication(
        controller.selectedApplicationId(),
        updateValues,
        {});
    QCOMPARE(updateRejectedSpy.count(), 1);
    QCOMPARE(updateQueuedSpy.count(), 0);
    QCOMPARE(controller.pendingSaveCount(), 0);
    mutationGate.endRemoval();

    QVERIFY(mutationGate.reserveCvImports(1));
    controller.setAllVisibleApplicationsChecked(true);
    QVERIFY(!controller.canDeleteApplications());
    controller.deleteCheckedApplications();
    QCOMPARE(deletionSpy.count(), 1);
    QCOMPARE(deletionSpy.first().at(0).toInt(), 0);
    QCOMPARE(deletionSpy.first().at(1).toInt(), controller.applicationCount());
    QCOMPARE(controller.checkedApplicationCount(), controller.applicationCount());
    mutationGate.releaseCvImports(1);
}

QTEST_GUILESS_MAIN(JobApplicationsControllerTest)

#include "JobApplicationsControllerTest.moc"

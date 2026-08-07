#include "directory/CompanyDirectoryController.hpp"
#include "directory/ContactListModel.hpp"
#include "jobs/JobApplicationListModel.hpp"
#include "jobs/JobApplicationsController.hpp"

#include "../support/AddJobTestFixture.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QUrl>
#include <QVariantMap>
#include <QtTest/QtTest>

namespace {

QVariantMap validFormValues(const QString& jobTitle)
{
    return {
        {QStringLiteral("jobTitle"), jobTitle},
        {QStringLiteral("jobUrl"), QString()},
        {QStringLiteral("companyName"), QStringLiteral("Example Company")},
        {QStringLiteral("workFormat"), QStringLiteral("Remote")},
        {QStringLiteral("status"), QStringLiteral("Applied")},
        {QStringLiteral("appliedDate"), QStringLiteral("2026-08-04")},
    };
}

QStringList persistedJobTitlesInInsertOrder(QSqlDatabase& database)
{
    QSqlQuery query{database};
    if (!query.exec(QStringLiteral("SELECT job_title FROM jobs ORDER BY rowid"))) {
        return {QStringLiteral("<query failed>")};
    }

    QStringList result;
    while (query.next()) {
        result.append(query.value(0).toString());
    }
    return result;
}

QStringList stagedFileNames(const testsupport::AddJobTestFixture& fixture)
{
    return QDir{fixture.storage_.paths().resumesDirectory()}.entryList(
        {QStringLiteral("*.part")},
        QDir::Files | QDir::NoDotAndDotDot);
}

}

class EndToEndIntegrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void persistsAndHydratesJobAcrossDatabaseReopen();
    void controllerPublishesSuccessfulCreation();
    void rapidSubmissionsPersistInFifoOrderWithExactStateTransitions();
    void failedRequestDoesNotBlockLaterQueuedRequest();
    void duplicateCvReuseLeavesNoStagedFiles();
    void activeCancellationContinuesWithNextQueuedRequest();
    void cancelAllRemovesQueuedWorkAndCleansActiveRequest();
    void controllerShutdownLeavesNoPartialState();
};

void EndToEndIntegrationTest::persistsAndHydratesJobAcrossDatabaseReopen()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    const auto sourcePath = storage.createFile();
    QString companyId;

    {
        SqliteDatabase database{storage.paths().databasePath()};
        CvRepository cvs{database.connection()};
        CompanyRepository companies{database.connection()};
        JobRepository jobs{database.connection()};
        CvManagedFileStore fileStore{storage.paths()};
        CvImportService importer{fileStore, cvs};
        AddJobService service{database.connection(), jobs, companies, importer};
        const auto result = service.create(
            testsupport::validJobDraft(),
            QUrl::fromLocalFile(sourcePath));
        QVERIFY2(result.success_, qPrintable(result.message_));
        companyId = result.company_.id_;
    }

    SqliteDatabase reopened{storage.paths().databasePath()};
    CvRepository cvs{reopened.connection()};
    CompanyRepository companies{reopened.connection()};
    JobRepository jobs{reopened.connection()};
    const auto storedCompanies = companies.findAll();
    const auto applications = jobs.findAll();
    QCOMPARE(storedCompanies.size(), 1);
    QCOMPARE(storedCompanies.first().id_, companyId);
    QCOMPARE(storedCompanies.first().name_, QStringLiteral("Example Company"));
    QCOMPARE(applications.size(), 1);
    QCOMPARE(applications.first().companyId_, companyId);
    QCOMPARE(cvs.findAll().size(), 1);
    QCOMPARE(applications.first().jobTitle_, QStringLiteral("Qt Developer"));

    JobApplicationListModel applicationModel{applications};
    ContactListModel contacts;
    CompanyDirectoryController companyDirectory{
        storedCompanies,
        applicationModel,
        contacts};
    QCOMPARE(companyDirectory.companyCount(), 1);
    QCOMPARE(companyDirectory.selectedCompanyId(), companyId);
    QCOMPARE(companyDirectory.linkedJobsModel()->rowCount(), 1);
}

void EndToEndIntegrationTest::controllerPublishesSuccessfulCreation()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.service_};
    ContactListModel contacts;
    CompanyDirectoryController companyDirectory{
        fixture.companyRepository_.findAll(),
        controller.jobApplicationListModel(),
        contacts};
    QObject::connect(
        &controller,
        &JobApplicationsController::companyResolved,
        &companyDirectory,
        &CompanyDirectoryController::publishCompany);
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationQueued};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy createdSpy{&controller, &JobApplicationsController::applicationCreated};
    QSignalSpy companySpy{&controller, &JobApplicationsController::companyResolved};
    QSignalSpy failedSpy{&controller, &JobApplicationsController::saveFailed};

    controller.createApplication(
        validFormValues(QStringLiteral("Qt Developer")),
        QUrl::fromLocalFile(fixture.storage_.createFile()));

    QTRY_COMPARE(completedSpy.count(), 1);
    QCOMPARE(queuedSpy.count(), 1);
    QCOMPARE(createdSpy.count(), 1);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(companySpy.count(), 1);
    QVERIFY(completedSpy.first().at(2).toBool());
    QVERIFY(!completedSpy.first().at(3).toString().isEmpty());
    QCOMPARE(controller.applicationCount(), 1);
    QCOMPARE(companyDirectory.companyCount(), 1);
    QVERIFY(!companyDirectory.selectedCompanyId().isEmpty());
    QCOMPARE(
        companyDirectory.selectedCompany().value(QStringLiteral("name")).toString(),
        QStringLiteral("Example Company"));
    QCOMPARE(companyDirectory.linkedJobsModel()->rowCount(), 1);
    QCOMPARE(controller.pendingSaveCount(), 0);
    QVERIFY(!controller.saving());
}

void EndToEndIntegrationTest::rapidSubmissionsPersistInFifoOrderWithExactStateTransitions()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.service_};
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationQueued};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy createdSpy{&controller, &JobApplicationsController::applicationCreated};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    QSignalSpy failedSpy{&controller, &JobApplicationsController::saveFailed};
    QList<int> pendingCounts;
    QList<bool> savingStates;
    QObject::connect(
        &controller,
        &JobApplicationsController::pendingSaveCountChanged,
        &controller,
        [&controller, &pendingCounts]() { pendingCounts.append(controller.pendingSaveCount()); });
    QObject::connect(
        &controller,
        &JobApplicationsController::savingChanged,
        &controller,
        [&controller, &savingStates]() { savingStates.append(controller.saving()); });
    const auto sourcePath = fixture.storage_.createFile();

    controller.createApplication(
        validFormValues(QStringLiteral("First Role")),
        QUrl::fromLocalFile(sourcePath));
    controller.createApplication(
        validFormValues(QStringLiteral("Second Role")),
        QUrl::fromLocalFile(sourcePath));
    controller.createApplication(
        validFormValues(QStringLiteral("Third Role")),
        QUrl::fromLocalFile(sourcePath));

    QCOMPARE(controller.pendingSaveCount(), 3);
    QVERIFY(controller.saving());
    QCOMPARE(queuedSpy.count(), 3);
    QCOMPARE(pendingCounts, QList<int>({1, 2, 3}));
    QCOMPARE(savingStates, QList<bool>({true}));

    QTRY_COMPARE(drainedSpy.count(), 1);

    QCOMPARE(completedSpy.count(), 3);
    QCOMPARE(createdSpy.count(), 3);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(pendingCounts, QList<int>({1, 2, 3, 2, 1, 0}));
    QCOMPARE(savingStates, QList<bool>({true, false}));
    QCOMPARE(completedSpy.at(0).at(0).toULongLong(), quint64(1));
    QCOMPARE(completedSpy.at(1).at(0).toULongLong(), quint64(2));
    QCOMPARE(completedSpy.at(2).at(0).toULongLong(), quint64(3));
    QCOMPARE(completedSpy.at(0).at(1).toString(), QStringLiteral("First Role"));
    QCOMPARE(completedSpy.at(1).at(1).toString(), QStringLiteral("Second Role"));
    QCOMPARE(completedSpy.at(2).at(1).toString(), QStringLiteral("Third Role"));
    QVERIFY(completedSpy.at(0).at(2).toBool());
    QVERIFY(completedSpy.at(1).at(2).toBool());
    QVERIFY(completedSpy.at(2).at(2).toBool());
    QCOMPARE(
        persistedJobTitlesInInsertOrder(fixture.database_.connection()),
        QStringList({
            QStringLiteral("First Role"),
            QStringLiteral("Second Role"),
            QStringLiteral("Third Role"),
        }));
    QCOMPARE(controller.pendingSaveCount(), 0);
    QVERIFY(!controller.saving());
}

void EndToEndIntegrationTest::failedRequestDoesNotBlockLaterQueuedRequest()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.service_};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy createdSpy{&controller, &JobApplicationsController::applicationCreated};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    QSignalSpy failedSpy{&controller, &JobApplicationsController::saveFailed};
    const auto missingPath = QDir{fixture.storage_.rootPath()}.filePath(
        QStringLiteral("missing.pdf"));
    const auto validPath = fixture.storage_.createFile(QStringLiteral("later.pdf"));

    controller.createApplication(
        validFormValues(QStringLiteral("Unavailable CV Role")),
        QUrl::fromLocalFile(missingPath));
    controller.createApplication(
        validFormValues(QStringLiteral("Later Valid Role")),
        QUrl::fromLocalFile(validPath));

    QTRY_COMPARE(drainedSpy.count(), 1);

    QCOMPARE(completedSpy.count(), 2);
    QCOMPARE(createdSpy.count(), 1);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(completedSpy.at(0).at(1).toString(), QStringLiteral("Unavailable CV Role"));
    QVERIFY(!completedSpy.at(0).at(2).toBool());
    QVERIFY(!completedSpy.at(0).at(3).toString().isEmpty());
    QCOMPARE(completedSpy.at(1).at(1).toString(), QStringLiteral("Later Valid Role"));
    QVERIFY(completedSpy.at(1).at(2).toBool());
    QCOMPARE(
        persistedJobTitlesInInsertOrder(fixture.database_.connection()),
        QStringList({QStringLiteral("Later Valid Role")}));
}

void EndToEndIntegrationTest::duplicateCvReuseLeavesNoStagedFiles()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.service_};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    const auto sourcePath = fixture.storage_.createFile(QStringLiteral("shared.pdf"));

    controller.createApplication(
        validFormValues(QStringLiteral("First Shared CV Role")),
        QUrl::fromLocalFile(sourcePath));
    controller.createApplication(
        validFormValues(QStringLiteral("Second Shared CV Role")),
        QUrl::fromLocalFile(sourcePath));

    QTRY_COMPARE(drainedSpy.count(), 1);

    QCOMPARE(completedSpy.count(), 2);
    QVERIFY(completedSpy.at(0).at(2).toBool());
    QVERIFY(completedSpy.at(1).at(2).toBool());
    QCOMPARE(fixture.jobRepository_.findAll().size(), 2);
    QCOMPARE(fixture.cvRepository_.findAll().size(), 1);
    QCOMPARE(
        QDir{fixture.storage_.paths().resumesDirectory()}.entryList(
            QDir::Files | QDir::NoDotAndDotDot).size(),
        1);
    QVERIFY(stagedFileNames(fixture).isEmpty());
}

void EndToEndIntegrationTest::activeCancellationContinuesWithNextQueuedRequest()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.service_};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    QSignalSpy failedSpy{&controller, &JobApplicationsController::saveFailed};
    const auto activePath = fixture.storage_.createFile(
        QStringLiteral("active.pdf"),
        QByteArray(8 * 1024 * 1024, 'a'));
    const auto queuedPath = fixture.storage_.createFile(QStringLiteral("queued.pdf"));

    controller.createApplication(
        validFormValues(QStringLiteral("Canceled Active Role")),
        QUrl::fromLocalFile(activePath));
    controller.createApplication(
        validFormValues(QStringLiteral("Queued Role")),
        QUrl::fromLocalFile(queuedPath));
    controller.cancelCreateApplication();

    QTRY_COMPARE(drainedSpy.count(), 1);

    QCOMPARE(completedSpy.count(), 2);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(completedSpy.at(0).at(1).toString(), QStringLiteral("Canceled Active Role"));
    QVERIFY(!completedSpy.at(0).at(2).toBool());
    QVERIFY(completedSpy.at(0).at(3).toString().contains(
        QStringLiteral("canceled"),
        Qt::CaseInsensitive));
    QCOMPARE(completedSpy.at(1).at(1).toString(), QStringLiteral("Queued Role"));
    QVERIFY(completedSpy.at(1).at(2).toBool());
    QCOMPARE(
        persistedJobTitlesInInsertOrder(fixture.database_.connection()),
        QStringList({QStringLiteral("Queued Role")}));
    QVERIFY(stagedFileNames(fixture).isEmpty());
}

void EndToEndIntegrationTest::cancelAllRemovesQueuedWorkAndCleansActiveRequest()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.service_};
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationQueued};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy createdSpy{&controller, &JobApplicationsController::applicationCreated};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    QSignalSpy failedSpy{&controller, &JobApplicationsController::saveFailed};
    QList<int> pendingCounts;
    QList<bool> savingStates;
    QObject::connect(
        &controller,
        &JobApplicationsController::pendingSaveCountChanged,
        &controller,
        [&controller, &pendingCounts]() { pendingCounts.append(controller.pendingSaveCount()); });
    QObject::connect(
        &controller,
        &JobApplicationsController::savingChanged,
        &controller,
        [&controller, &savingStates]() { savingStates.append(controller.saving()); });
    const auto activePath = fixture.storage_.createFile(
        QStringLiteral("cancel-all-active.pdf"),
        QByteArray(8 * 1024 * 1024, 'c'));
    const auto queuedPath = fixture.storage_.createFile(QStringLiteral("cancel-all-queued.pdf"));

    controller.createApplication(
        validFormValues(QStringLiteral("Active Role")),
        QUrl::fromLocalFile(activePath));
    controller.createApplication(
        validFormValues(QStringLiteral("Queued Role 1")),
        QUrl::fromLocalFile(queuedPath));
    controller.createApplication(
        validFormValues(QStringLiteral("Queued Role 2")),
        QUrl::fromLocalFile(queuedPath));
    controller.cancelAllCreateApplications();

    QCOMPARE(queuedSpy.count(), 3);
    QCOMPARE(controller.pendingSaveCount(), 1);
    QCOMPARE(pendingCounts, QList<int>({1, 2, 3, 1}));
    QCOMPARE(savingStates, QList<bool>({true}));

    QTRY_COMPARE(drainedSpy.count(), 1);

    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(createdSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(pendingCounts, QList<int>({1, 2, 3, 1, 0}));
    QCOMPARE(savingStates, QList<bool>({true, false}));
    QCOMPARE(controller.pendingSaveCount(), 0);
    QVERIFY(!controller.saving());
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QVERIFY(fixture.companyRepository_.findAll().isEmpty());
    QVERIFY(fixture.cvRepository_.findAll().isEmpty());
    QCOMPARE(
        QDir{fixture.storage_.paths().resumesDirectory()}.entryList(
            QDir::Files | QDir::NoDotAndDotDot).size(),
        0);
}

void EndToEndIntegrationTest::controllerShutdownLeavesNoPartialState()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto activePath = fixture.storage_.createFile(
        QStringLiteral("shutdown-active.pdf"),
        QByteArray(8 * 1024 * 1024, 's'));
    const auto queuedPath = fixture.storage_.createFile(QStringLiteral("shutdown-queued.pdf"));

    {
        JobApplicationsController controller{{}, fixture.service_};
        controller.createApplication(
            validFormValues(QStringLiteral("Shutdown Active Role")),
            QUrl::fromLocalFile(activePath));
        controller.createApplication(
            validFormValues(QStringLiteral("Shutdown Queued Role")),
            QUrl::fromLocalFile(queuedPath));
    }

    QCoreApplication::processEvents();
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QVERIFY(fixture.companyRepository_.findAll().isEmpty());
    QVERIFY(fixture.cvRepository_.findAll().isEmpty());
    QCOMPARE(
        QDir{fixture.storage_.paths().resumesDirectory()}.entryList(
            QDir::Files | QDir::NoDotAndDotDot).size(),
        0);
}

QTEST_GUILESS_MAIN(EndToEndIntegrationTest)

#include "EndToEndIntegrationTest.moc"

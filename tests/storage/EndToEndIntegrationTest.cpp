#include "directory/CompanyDirectoryController.hpp"
#include "directory/ContactListModel.hpp"
#include "jobs/JobApplicationListModel.hpp"
#include "jobs/JobApplicationsController.hpp"

#include "../support/AddJobTestFixture.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QSignalSpy>
#include <QUrl>
#include <QtTest/QtTest>

class EndToEndIntegrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void persistsAndHydratesJobAcrossDatabaseReopen();
    void controllerPublishesSuccessfulCreation();
    void controllerCancellationLeavesNoDurableState();
    void controllerShutdownCleansLatePreparation();
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
    QSignalSpy createdSpy{&controller, &JobApplicationsController::applicationCreated};
    QSignalSpy companySpy{&controller, &JobApplicationsController::companyResolved};
    QSignalSpy failedSpy{&controller, &JobApplicationsController::saveFailed};

    controller.createApplication(
        {
            {QStringLiteral("jobTitle"), QStringLiteral("Qt Developer")},
            {QStringLiteral("companyName"), QStringLiteral("Example Company")},
            {QStringLiteral("status"), QStringLiteral("Applied")},
            {QStringLiteral("appliedDate"), QStringLiteral("2026-07-09")},
        },
        QUrl::fromLocalFile(fixture.storage_.createFile()));

    QTRY_COMPARE(createdSpy.count(), 1);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(companySpy.count(), 1);
    QCOMPARE(controller.applicationCount(), 1);
    QCOMPARE(companyDirectory.companyCount(), 1);
    QVERIFY(!companyDirectory.selectedCompanyId().isEmpty());
    QCOMPARE(
        companyDirectory.selectedCompany().value(QStringLiteral("name")).toString(),
        QStringLiteral("Example Company"));
    QCOMPARE(companyDirectory.linkedJobsModel()->rowCount(), 1);
    QVERIFY(!controller.saving());
}

void EndToEndIntegrationTest::controllerCancellationLeavesNoDurableState()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.service_};
    QSignalSpy failedSpy{&controller, &JobApplicationsController::saveFailed};
    const auto sourcePath = fixture.storage_.createFile(
        QStringLiteral("cancel-controller.pdf"),
        QByteArray(8 * 1024 * 1024, 'c'));

    controller.createApplication(
        {
            {QStringLiteral("jobTitle"), QStringLiteral("Qt Developer")},
            {QStringLiteral("companyName"), QStringLiteral("Example Company")},
            {QStringLiteral("status"), QStringLiteral("Applied")},
            {QStringLiteral("appliedDate"), QStringLiteral("2026-07-09")},
        },
        QUrl::fromLocalFile(sourcePath));
    controller.cancelCreateApplication();

    QTRY_VERIFY(!controller.saving());
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QVERIFY(fixture.companyRepository_.findAll().isEmpty());
    QVERIFY(fixture.cvRepository_.findAll().isEmpty());
    QCOMPARE(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).size(), 0);
}

void EndToEndIntegrationTest::controllerShutdownCleansLatePreparation()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto sourcePath = fixture.storage_.createFile(
        QStringLiteral("shutdown.pdf"),
        QByteArray(8 * 1024 * 1024, 's'));

    {
        JobApplicationsController controller{{}, fixture.service_};
        controller.createApplication(
            {
                {QStringLiteral("jobTitle"), QStringLiteral("Qt Developer")},
                {QStringLiteral("companyName"), QStringLiteral("Example Company")},
                {QStringLiteral("status"), QStringLiteral("Applied")},
                {QStringLiteral("appliedDate"), QStringLiteral("2026-07-09")},
            },
            QUrl::fromLocalFile(sourcePath));
    }

    QCoreApplication::processEvents();
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QVERIFY(fixture.companyRepository_.findAll().isEmpty());
    QVERIFY(fixture.cvRepository_.findAll().isEmpty());
    QCOMPARE(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).size(), 0);
}

QTEST_GUILESS_MAIN(EndToEndIntegrationTest)

#include "EndToEndIntegrationTest.moc"

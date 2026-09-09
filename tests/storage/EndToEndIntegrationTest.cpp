#include "directory/CompanyDirectoryController.hpp"
#include "directory/ContactListModel.hpp"
#include "jobs/JobApplicationListModel.hpp"
#include "jobs/JobApplicationsController.hpp"
#include "jobs/JobApplicationValidator.hpp"
#include "cvs/CvImportWorker.hpp"

#include "../support/AddJobTestFixture.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <QtTest/QtTest>

namespace {

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

QStringList stagedFileNames(const testsupport::AddJobWorkerTestFixture& fixture)
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
    void initTestCase();
    void persistsAndHydratesJobAcrossDatabaseReopen();
    void controllerPublishesSuccessfulCreation();
    void rapidSubmissionsPersistInFifoOrderWithExactStateTransitions();
    void invalidRequestFailsSynchronouslyWithoutMutationAndContinues();
    void failedRequestDoesNotBlockLaterQueuedRequest();
    void duplicateCvReuseLeavesNoStagedFiles();
    void concurrentJobAndCvImports_data();
    void concurrentJobAndCvImports();
    void workerInitializationFailureCleansConnectionAndRetries();
    void databaseLockFailureDoesNotBlockLaterRequest();
    void largeCvProcessingKeepsGuiEventLoopResponsive();
    void activeCancellationContinuesWithNextQueuedRequest();
    void cancelAllRemovesQueuedWorkAndCleansActiveRequest();
    void controllerShutdownLeavesNoPartialState();
};

void EndToEndIntegrationTest::initTestCase()
{
    qRegisterMetaType<Company>();
}

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
        CvMutationQueue cvMutationQueue;
        AddJobService service{database.connection(), jobs, companies, importer, cvMutationQueue};
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
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.worker_};
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

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Qt Developer")),
        QUrl::fromLocalFile(fixture.storage_.createFile()));

    QTRY_COMPARE(completedSpy.count(), 1);
    QCOMPARE(queuedSpy.count(), 1);
    QCOMPARE(createdSpy.count(), 1);
    QCOMPARE(companySpy.count(), 1);
    const auto publishedCompany = companySpy.first().at(0).value<Company>();
    const auto storedCompanies = fixture.companyRepository_.findAll();
    QCOMPARE(storedCompanies.size(), 1);
    const auto& storedCompany = storedCompanies.first();
    QCOMPARE(publishedCompany.id_, storedCompany.id_);
    QCOMPARE(publishedCompany.name_, storedCompany.name_);
    QCOMPARE(publishedCompany.website_, storedCompany.website_);
    QCOMPARE(publishedCompany.description_, storedCompany.description_);
    QCOMPARE(publishedCompany.notes_, storedCompany.notes_);
    QCOMPARE(publishedCompany.createdAt_, storedCompany.createdAt_);
    QCOMPARE(publishedCompany.updatedAt_, storedCompany.updatedAt_);
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
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.worker_};
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationQueued};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy createdSpy{&controller, &JobApplicationsController::applicationCreated};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    QList<int> pendingCounts;
    QList<bool> savingStates;
    QStringList eventOrder;
    const auto initialConnectionCount = QSqlDatabase::connectionNames().size();
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
    QObject::connect(
        &controller,
        &JobApplicationsController::applicationQueued,
        &controller,
        [&eventOrder](quint64 operationId) {
            eventOrder.append(QStringLiteral("queued-%1").arg(operationId));
        });
    QObject::connect(
        &controller,
        &JobApplicationsController::applicationSaveCompleted,
        &controller,
        [&eventOrder](quint64 operationId, const QString&, bool, const QString&) {
            eventOrder.append(QStringLiteral("completed-%1").arg(operationId));
        });
    const auto sourcePath = fixture.storage_.createFile();

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("First Role")),
        QUrl::fromLocalFile(sourcePath));
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Second Role")),
        QUrl::fromLocalFile(sourcePath));
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Third Role")),
        QUrl::fromLocalFile(sourcePath));

    QCOMPARE(controller.pendingSaveCount(), 3);
    QVERIFY(controller.saving());
    QCOMPARE(queuedSpy.count(), 3);
    QCOMPARE(pendingCounts, QList<int>({1, 2, 3}));
    QCOMPARE(savingStates, QList<bool>({true}));

    QTRY_COMPARE(drainedSpy.count(), 1);

    QCOMPARE(completedSpy.count(), 3);
    QCOMPARE(createdSpy.count(), 3);
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
        eventOrder,
        QStringList({
            QStringLiteral("queued-1"),
            QStringLiteral("queued-2"),
            QStringLiteral("queued-3"),
            QStringLiteral("completed-1"),
            QStringLiteral("completed-2"),
            QStringLiteral("completed-3"),
        }));
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount + 1);
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

void EndToEndIntegrationTest::invalidRequestFailsSynchronouslyWithoutMutationAndContinues()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.worker_};
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationQueued};
    QSignalSpy failedSpy{&controller, &JobApplicationsController::saveFailed};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    QStringList eventOrder;
    bool failureHadNoDatabaseOrFileMutation = false;
    const auto connectionCountBeforeRequests = QSqlDatabase::connectionNames().size();
    bool failureCreatedNoWorkerConnection = false;

    QObject::connect(
        &controller,
        &JobApplicationsController::applicationQueued,
        &controller,
        [&eventOrder](quint64 operationId) {
            eventOrder.append(QStringLiteral("queued-%1").arg(operationId));
        });
    QObject::connect(
        &controller,
        &JobApplicationsController::saveFailed,
        &controller,
        [
            &fixture,
            &eventOrder,
            &failureHadNoDatabaseOrFileMutation,
            connectionCountBeforeRequests,
            &failureCreatedNoWorkerConnection](
            const QVariantMap&,
            const QString&) {
            eventOrder.append(QStringLiteral("failed"));
            failureHadNoDatabaseOrFileMutation = fixture.jobRepository_.findAll().isEmpty()
                && fixture.companyRepository_.findAll().isEmpty()
                && fixture.cvRepository_.findAll().isEmpty()
                && QDir{fixture.storage_.paths().resumesDirectory()}
                    .entryList(QDir::Files | QDir::NoDotAndDotDot)
                    .isEmpty();
            failureCreatedNoWorkerConnection =
                QSqlDatabase::connectionNames().size() == connectionCountBeforeRequests;
        });
    QObject::connect(
        &controller,
        &JobApplicationsController::applicationSaveCompleted,
        &controller,
        [&eventOrder](quint64 operationId, const QString&, bool, const QString&) {
            eventOrder.append(QStringLiteral("completed-%1").arg(operationId));
        });

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
    QCOMPARE(queuedSpy.count(), 0);
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(controller.pendingSaveCount(), 0);
    QVERIFY(!controller.saving());
    QVERIFY(!fixture.worker_.isRunning());
    QVERIFY(failureHadNoDatabaseOrFileMutation);
    QVERIFY(failureCreatedNoWorkerConnection);

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Valid After Failure")),
        QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("after-failure.pdf"))));

    QCOMPARE(queuedSpy.count(), 1);
    QCOMPARE(queuedSpy.first().at(0).toULongLong(), quint64(1));
    QCOMPARE(controller.pendingSaveCount(), 1);
    QTRY_COMPARE(drainedSpy.count(), 1);

    const auto errors = failedSpy.first().at(0).toMap();
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
        QVERIFY2(errors.contains(field), qPrintable(field));
    }
    QCOMPARE(completedSpy.count(), 1);
    QCOMPARE(completedSpy.first().at(0).toULongLong(), quint64(1));
    QVERIFY(completedSpy.first().at(2).toBool());
    QCOMPARE(
        eventOrder,
        QStringList({
            QStringLiteral("failed"),
            QStringLiteral("queued-1"),
            QStringLiteral("completed-1"),
        }));
    QCOMPARE(
        persistedJobTitlesInInsertOrder(fixture.database_.connection()),
        QStringList({QStringLiteral("Valid After Failure")}));
    QVERIFY(stagedFileNames(fixture).isEmpty());
}

void EndToEndIntegrationTest::failedRequestDoesNotBlockLaterQueuedRequest()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.worker_};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy createdSpy{&controller, &JobApplicationsController::applicationCreated};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    const auto missingPath = QDir{fixture.storage_.rootPath()}.filePath(
        QStringLiteral("missing.pdf"));
    const auto validPath = fixture.storage_.createFile(QStringLiteral("later.pdf"));

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Unavailable CV Role")),
        QUrl::fromLocalFile(missingPath));
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Later Valid Role")),
        QUrl::fromLocalFile(validPath));

    QTRY_COMPARE(drainedSpy.count(), 1);

    QCOMPARE(completedSpy.count(), 2);
    QCOMPARE(createdSpy.count(), 1);
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
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.worker_};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    const auto sourcePath = fixture.storage_.createFile(QStringLiteral("shared.pdf"));

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("First Shared CV Role")),
        QUrl::fromLocalFile(sourcePath));
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Second Shared CV Role")),
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

void EndToEndIntegrationTest::concurrentJobAndCvImports_data()
{
    QTest::addColumn<bool>("cvFirst");
    QTest::addColumn<bool>("rejectJob");
    QTest::newRow("job-first") << false << false;
    QTest::newRow("cv-first") << true << false;
    QTest::newRow("job-rollback") << false << true;
    QTest::newRow("cv-first-job-rollback") << true << true;
}

void EndToEndIntegrationTest::concurrentJobAndCvImports()
{
    QFETCH(bool, cvFirst);
    QFETCH(bool, rejectJob);
    testsupport::AddJobWorkerTestFixture fixture;
    CvImportWorker importer{fixture.storage_.paths().dataDirectory(), fixture.cvMutationQueue_};
    if (rejectJob) {
        QSqlQuery trigger{fixture.database_.connection()};
        QVERIFY(trigger.exec(QStringLiteral(
            "CREATE TRIGGER reject_job BEFORE INSERT ON jobs "
            "BEGIN SELECT RAISE(FAIL, 'forced concurrent job failure'); END")));
    }
    const auto source = QUrl::fromLocalFile(fixture.storage_.createFile(
        QStringLiteral("shared.pdf"), QByteArray(16 * 1024 * 1024, 's')));
    const auto draft = JobApplicationValidator::preflight(testsupport::validJobDraft(), true);
    QVERIFY(draft.isValid());
    QSignalSpy jobCompleted{&fixture.worker_, &JobSaveWorker::saveCompleted};
    QSignalSpy cvCompleted{&importer, &CvImportWorker::importCompleted};
    auto held = fixture.cvMutationQueue_.acquire({});
    const auto submitJob = [&] {
        fixture.worker_.submit(AddJobRequest{
            1, draft.draft_, source, std::make_shared<CancellationState>()});
    };
    const auto submitCv = [&] {
        importer.submit(CvImportRequest{1, source, std::make_shared<CancellationState>()});
    };
    if (cvFirst) {
        submitCv();
        submitJob();
    } else {
        submitJob();
        submitCv();
    }
    // Both worker requests are active while mutation admission is withheld.
    QVERIFY(fixture.worker_.isRunning());
    QVERIFY(importer.isRunning());
    QVERIFY(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).isEmpty());
    held = {};
    QTRY_COMPARE_WITH_TIMEOUT(jobCompleted.count(), 1, 10000);
    QTRY_COMPARE_WITH_TIMEOUT(cvCompleted.count(), 1, 10000);
    const auto job = jobCompleted.first().first().value<AddJobSaveOutcome>().result_;
    const auto cv = cvCompleted.first().first().value<CvImportSaveOutcome>();
    QVERIFY2(cv.success_, qPrintable(cv.message_));
    QCOMPARE(job.success_, !rejectJob);
    if (!rejectJob) {
        QCOMPARE(job.cvDocument_.id_, cv.document_.id_);
        QVERIFY((job.cvImportDisposition_ == CvImportDisposition::Inserted
                    && cv.disposition_ == CvImportDisposition::ExistingActive)
            || (job.cvImportDisposition_ == CvImportDisposition::ExistingActive
                    && cv.disposition_ == CvImportDisposition::Inserted));
    } else {
        QCOMPARE(cv.disposition_, CvImportDisposition::Inserted);
    }
    QCOMPARE(fixture.jobRepository_.findAll().size(), rejectJob ? 0 : 1);
    QCOMPARE(fixture.companyRepository_.findAll().size(), rejectJob ? 0 : 1);
    QCOMPARE(fixture.cvRepository_.findAll().size(), 1);
    QCOMPARE(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).size(), 1);
    QVERIFY(stagedFileNames(fixture).isEmpty());
}

void EndToEndIntegrationTest::workerInitializationFailureCleansConnectionAndRetries()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    const auto databasePath = storage.paths().databasePath();
    QVERIFY(QDir{}.mkpath(databasePath));
    const auto initialConnectionCount = QSqlDatabase::connectionNames().size();

    CvMutationQueue cvMutationQueue;
    JobSaveWorker worker{storage.paths().dataDirectory(), cvMutationQueue};
    JobApplicationsController controller{{}, worker};
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationQueued};
    QSignalSpy failedSpy{&controller, &JobApplicationsController::saveFailed};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    const auto sourcePath = storage.createFile(QStringLiteral("worker-retry.pdf"));

    auto invalidValues = testsupport::validJobFormValues(
        QStringLiteral("Invalid Before Initialization"));
    invalidValues.insert(QStringLiteral("jobTitle"), QStringLiteral(" "));
    controller.createApplication(invalidValues, QUrl::fromLocalFile(sourcePath));

    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(failedSpy.first().at(0).toMap().contains(QStringLiteral("jobTitle")));
    QCOMPARE(queuedSpy.count(), 0);
    QCOMPARE(completedSpy.count(), 0);
    QVERIFY(!worker.isRunning());
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Initialization Failure")),
        QUrl::fromLocalFile(sourcePath));

    QTRY_COMPARE(completedSpy.count(), 1);
    QCOMPARE(queuedSpy.count(), 1);
    QCOMPARE(completedSpy.first().at(0).toULongLong(), quint64(1));
    QCOMPARE(completedSpy.first().at(1).toString(), QStringLiteral("Initialization Failure"));
    QVERIFY(!completedSpy.first().at(2).toBool());
    QVERIFY(!completedSpy.first().at(3).toString().isEmpty());
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);
    QCOMPARE(
        QDir{storage.paths().resumesDirectory()}.entryList(QDir::Files).size(),
        0);

    QVERIFY(QDir{databasePath}.removeRecursively());
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Initialization Retry")),
        QUrl::fromLocalFile(sourcePath));

    QTRY_COMPARE(completedSpy.count(), 2);
    QCOMPARE(queuedSpy.count(), 2);
    QCOMPARE(completedSpy.at(1).at(0).toULongLong(), quint64(2));
    QCOMPARE(completedSpy.at(1).at(1).toString(), QStringLiteral("Initialization Retry"));
    QVERIFY(completedSpy.at(1).at(2).toBool());
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount + 1);

    worker.shutdown();
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);

    SqliteDatabase database{databasePath};
    JobRepository jobs{database.connection()};
    QCOMPARE(jobs.findAll().size(), 1);
    QCOMPARE(jobs.findAll().first().jobTitle_, QStringLiteral("Initialization Retry"));
}

void EndToEndIntegrationTest::databaseLockFailureDoesNotBlockLaterRequest()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.worker_};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Warm Worker")),
        QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("warm.pdf"))));
    QTRY_COMPARE(completedSpy.count(), 1);
    QVERIFY(completedSpy.first().at(2).toBool());
    completedSpy.clear();
    drainedSpy.clear();

    QSqlQuery exclusiveLock{fixture.database_.connection()};
    QVERIFY(exclusiveLock.exec(QStringLiteral("BEGIN EXCLUSIVE")));
    bool lockReleased = false;
    QObject::connect(
        &controller,
        &JobApplicationsController::applicationSaveCompleted,
        &controller,
        [&fixture, &lockReleased](
            quint64,
            const QString& jobTitle,
            bool,
            const QString&) {
            if (jobTitle != QStringLiteral("Locked Request")) {
                return;
            }
            QSqlQuery rollback{fixture.database_.connection()};
            lockReleased = rollback.exec(QStringLiteral("ROLLBACK"));
        });

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Locked Request")),
        QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("locked.pdf"))));
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("After Lock Failure")),
        QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("after-lock.pdf"))));

    QTRY_COMPARE_WITH_TIMEOUT(drainedSpy.count(), 1, 10000);
    QCOMPARE(completedSpy.count(), 2);
    QCOMPARE(completedSpy.at(0).at(1).toString(), QStringLiteral("Locked Request"));
    QVERIFY(!completedSpy.at(0).at(2).toBool());
    QVERIFY(lockReleased);
    QCOMPARE(completedSpy.at(1).at(1).toString(), QStringLiteral("After Lock Failure"));
    QVERIFY(completedSpy.at(1).at(2).toBool());
    QCOMPARE(
        persistedJobTitlesInInsertOrder(fixture.database_.connection()),
        QStringList({
            QStringLiteral("Warm Worker"),
            QStringLiteral("After Lock Failure"),
        }));
    QVERIFY(stagedFileNames(fixture).isEmpty());
}

void EndToEndIntegrationTest::largeCvProcessingKeepsGuiEventLoopResponsive()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.worker_};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    const auto sourcePath = fixture.storage_.createFile(
        QStringLiteral("large.pdf"),
        QByteArray(64 * 1024 * 1024, 'g'));
    QVERIFY(!sourcePath.isEmpty());

    int guiTimerTicks = 0;
    QTimer guiTimer;
    guiTimer.setInterval(1);
    QObject::connect(&guiTimer, &QTimer::timeout, &controller, [&controller, &guiTimerTicks]() {
        if (controller.pendingSaveCount() > 0) {
            ++guiTimerTicks;
        }
    });
    guiTimer.start();

    QElapsedTimer callTimer;
    callTimer.start();
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Large CV Role")),
        QUrl::fromLocalFile(sourcePath));
    QVERIFY2(callTimer.elapsed() < 1000, "Save blocked the GUI thread before returning.");

    QTRY_COMPARE_WITH_TIMEOUT(drainedSpy.count(), 1, 10000);
    guiTimer.stop();
    QCOMPARE(completedSpy.count(), 1);
    QVERIFY(completedSpy.first().at(2).toBool());
    QVERIFY2(guiTimerTicks > 0, "The GUI event loop did not advance during CV processing.");
}

void EndToEndIntegrationTest::activeCancellationContinuesWithNextQueuedRequest()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.worker_};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
    const auto activePath = fixture.storage_.createFile(
        QStringLiteral("active.pdf"),
        QByteArray(32 * 1024 * 1024, 'a'));
    const auto queuedPath = fixture.storage_.createFile(QStringLiteral("queued.pdf"));

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Canceled Active Role")),
        QUrl::fromLocalFile(activePath));
    controller.cancelCreateApplication();
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Queued Role")),
        QUrl::fromLocalFile(queuedPath));

    QTRY_COMPARE(drainedSpy.count(), 1);

    QCOMPARE(completedSpy.count(), 2);
    QCOMPARE(completedSpy.first().at(0).toULongLong(), quint64(1));
    QCOMPARE(completedSpy.first().at(1).toString(), QStringLiteral("Canceled Active Role"));
    if (!completedSpy.first().at(2).toBool()) {
        QVERIFY(completedSpy.first().at(3).toString().contains(
            QStringLiteral("canceled"),
            Qt::CaseInsensitive));
    }
    const auto& queuedCompletion = completedSpy.last();
    QCOMPARE(queuedCompletion.at(0).toULongLong(), quint64(2));
    QCOMPARE(queuedCompletion.at(1).toString(), QStringLiteral("Queued Role"));
    QVERIFY(queuedCompletion.at(2).toBool());

    const auto firstRequestWasCommitted = completedSpy.first().at(2).toBool();
    QCOMPARE(
        persistedJobTitlesInInsertOrder(fixture.database_.connection()),
        firstRequestWasCommitted
            ? QStringList({
                  QStringLiteral("Canceled Active Role"),
                  QStringLiteral("Queued Role"),
              })
            : QStringList({QStringLiteral("Queued Role")}));
    QVERIFY(stagedFileNames(fixture).isEmpty());
}

void EndToEndIntegrationTest::cancelAllRemovesQueuedWorkAndCleansActiveRequest()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    JobApplicationsController controller{{}, fixture.worker_};
    QSignalSpy queuedSpy{&controller, &JobApplicationsController::applicationQueued};
    QSignalSpy completedSpy{&controller, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy createdSpy{&controller, &JobApplicationsController::applicationCreated};
    QSignalSpy drainedSpy{&controller, &JobApplicationsController::saveQueueDrained};
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
        QByteArray(32 * 1024 * 1024, 'c'));
    const auto queuedPath = fixture.storage_.createFile(QStringLiteral("cancel-all-queued.pdf"));

    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Active Role")),
        QUrl::fromLocalFile(activePath));
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Queued Role 1")),
        QUrl::fromLocalFile(queuedPath));
    controller.createApplication(
        testsupport::validJobFormValues(QStringLiteral("Queued Role 2")),
        QUrl::fromLocalFile(queuedPath));
    controller.cancelAllJobSaves();

    QCOMPARE(queuedSpy.count(), 3);
    QCOMPARE(controller.pendingSaveCount(), 1);
    QCOMPARE(pendingCounts, QList<int>({1, 2, 3, 1}));
    QCOMPARE(savingStates, QList<bool>({true}));

    QTRY_COMPARE(drainedSpy.count(), 1);

    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(pendingCounts, QList<int>({1, 2, 3, 1, 0}));
    QCOMPARE(savingStates, QList<bool>({true, false}));
    QCOMPARE(controller.pendingSaveCount(), 0);
    QVERIFY(!controller.saving());
    QVERIFY(createdSpy.count() == 0 || createdSpy.count() == 1);
    const auto applications = fixture.jobRepository_.findAll();
    QCOMPARE(applications.size(), createdSpy.count());
    QCOMPARE(fixture.companyRepository_.findAll().size(), createdSpy.count());
    QCOMPARE(fixture.cvRepository_.findAll().size(), createdSpy.count());
    QCOMPARE(
        QDir{fixture.storage_.paths().resumesDirectory()}.entryList(
            QDir::Files | QDir::NoDotAndDotDot).size(),
        createdSpy.count());
    if (!applications.isEmpty()) {
        QCOMPARE(applications.first().jobTitle_, QStringLiteral("Active Role"));
    }
    QVERIFY(stagedFileNames(fixture).isEmpty());
}

void EndToEndIntegrationTest::controllerShutdownLeavesNoPartialState()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto activePath = fixture.storage_.createFile(
        QStringLiteral("shutdown-active.pdf"),
        QByteArray(8 * 1024 * 1024, 's'));
    const auto queuedPath = fixture.storage_.createFile(QStringLiteral("shutdown-queued.pdf"));

    {
        JobApplicationsController controller{{}, fixture.worker_};
        controller.createApplication(
            testsupport::validJobFormValues(QStringLiteral("Shutdown Active Role")),
            QUrl::fromLocalFile(activePath));
        controller.createApplication(
            testsupport::validJobFormValues(QStringLiteral("Shutdown Queued Role")),
            QUrl::fromLocalFile(queuedPath));
    }

    fixture.worker_.shutdown();
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

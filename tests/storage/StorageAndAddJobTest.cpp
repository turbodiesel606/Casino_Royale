#include "cvs/CvImportService.h"
#include "cvs/CvRepository.h"
#include "jobs/AddJobService.h"
#include "jobs/JobApplicationDraft.h"
#include "jobs/JobApplicationsController.h"
#include "jobs/JobRepository.h"
#include "storage/SqliteDatabase.h"
#include "storage/StoragePaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

QString createCvFile(const QString& directory, const QString& name = QStringLiteral("resume.pdf"))
{
    const auto path = QDir(directory).filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    file.write("%PDF-1.4 JobTracker test CV");
    return path;
}

JobApplicationDraft validDraft()
{
    JobApplicationDraft draft;
    draft.jobTitle_ = QStringLiteral("Qt Developer");
    draft.companyName_ = QStringLiteral("Example Company");
    draft.jobUrl_ = QStringLiteral("https://example.com/jobs/qt");
    draft.status_ = QStringLiteral("Applied");
    draft.appliedDate_ = QStringLiteral("2026-07-09");
    draft.techStack_ = {QStringLiteral("Qt"), QStringLiteral(" C++ "), QStringLiteral("qt")};
    return draft;
}

}

class StorageAndAddJobTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsDatabaseAndMigratesOnce();
    void createsJobAndCopiesCv();
    void reusesCvWithSameContent();
    void persistsJobAcrossDatabaseReopen();
    void removesCopiedCvWhenJobInsertFails();
    void rejectsInvalidInputWithoutWriting();
    void controllerPublishesSuccessfulCreation();
};

void StorageAndAddJobTest::createsDatabaseAndMigratesOnce()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    {
        SqliteDatabase database(paths.databasePath());
        QSqlQuery version(database.connection());
        QVERIFY(version.exec(QStringLiteral("PRAGMA user_version")));
        QVERIFY(version.next());
        QCOMPARE(version.value(0).toInt(), 1);

        QSqlQuery foreignKeys(database.connection());
        QVERIFY(foreignKeys.exec(QStringLiteral("PRAGMA foreign_keys")));
        QVERIFY(foreignKeys.next());
        QCOMPARE(foreignKeys.value(0).toInt(), 1);
    }

    SqliteDatabase reopened(paths.databasePath());
    JobRepository jobs(reopened.connection());
    CvRepository cvs(reopened.connection());
    QVERIFY(jobs.findAll().isEmpty());
    QVERIFY(cvs.findAll().isEmpty());
}

void StorageAndAddJobTest::createsJobAndCopiesCv()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SqliteDatabase database(paths.databasePath());
    CvRepository cvs(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, importer);
    const auto sourcePath = createCvFile(temporaryDirectory.path());

    const auto result = service.create(validDraft(), QUrl::fromLocalFile(sourcePath));

    QVERIFY2(result.success_, qPrintable(result.message_));
    QVERIFY(result.cvWasInserted_);
    QCOMPARE(jobs.findAll().size(), 1);
    QCOMPARE(jobs.findAll().first().techStack_, QStringList({QStringLiteral("Qt"), QStringLiteral("C++")}));
    QCOMPARE(cvs.findAll().size(), 1);
    QVERIFY(QFileInfo::exists(QDir(paths.dataDirectory()).filePath(result.cvDocument_.relativePath_)));
}

void StorageAndAddJobTest::reusesCvWithSameContent()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SqliteDatabase database(paths.databasePath());
    CvRepository cvs(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, importer);
    const auto firstPath = createCvFile(temporaryDirectory.path(), QStringLiteral("first.pdf"));
    const auto secondPath = createCvFile(temporaryDirectory.path(), QStringLiteral("second.pdf"));

    const auto first = service.create(validDraft(), QUrl::fromLocalFile(firstPath));
    auto secondDraft = validDraft();
    secondDraft.jobTitle_ = QStringLiteral("Senior Qt Developer");
    const auto second = service.create(secondDraft, QUrl::fromLocalFile(secondPath));

    QVERIFY(first.success_);
    QVERIFY(second.success_);
    QVERIFY(!second.cvWasInserted_);
    QCOMPARE(first.cvDocument_.id_, second.cvDocument_.id_);
    QCOMPARE(cvs.findAll().size(), 1);
    QCOMPARE(jobs.findAll().size(), 2);
    QCOMPARE(QDir(paths.resumesDirectory()).entryList(QDir::Files).size(), 1);
}

void StorageAndAddJobTest::persistsJobAcrossDatabaseReopen()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    const auto sourcePath = createCvFile(temporaryDirectory.path());

    {
        SqliteDatabase database(paths.databasePath());
        CvRepository cvs(database.connection());
        JobRepository jobs(database.connection());
        CvImportService importer(paths, cvs);
        AddJobService service(database.connection(), jobs, importer);
        QVERIFY(service.create(validDraft(), QUrl::fromLocalFile(sourcePath)).success_);
    }

    SqliteDatabase reopened(paths.databasePath());
    CvRepository cvs(reopened.connection());
    JobRepository jobs(reopened.connection());
    QCOMPARE(jobs.findAll().size(), 1);
    QCOMPARE(cvs.findAll().size(), 1);
    QCOMPARE(jobs.findAll().first().jobTitle_, QStringLiteral("Qt Developer"));
}

void StorageAndAddJobTest::removesCopiedCvWhenJobInsertFails()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SqliteDatabase database(paths.databasePath());
    CvRepository cvs(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, importer);
    QSqlQuery trigger(database.connection());
    QVERIFY(trigger.exec(QStringLiteral(
        "CREATE TRIGGER reject_job BEFORE INSERT ON jobs "
        "BEGIN SELECT RAISE(FAIL, 'forced job failure'); END")));

    const auto result = service.create(
        validDraft(),
        QUrl::fromLocalFile(createCvFile(temporaryDirectory.path())));

    QVERIFY(!result.success_);
    QVERIFY(jobs.findAll().isEmpty());
    QVERIFY(cvs.findAll().isEmpty());
    QVERIFY(QDir(paths.resumesDirectory()).entryList(QDir::Files).isEmpty());
}

void StorageAndAddJobTest::rejectsInvalidInputWithoutWriting()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SqliteDatabase database(paths.databasePath());
    CvRepository cvs(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, importer);
    auto draft = validDraft();
    draft.jobTitle_.clear();
    draft.jobUrl_ = QStringLiteral("file:///not-a-job");

    const auto result = service.create(draft, {});

    QVERIFY(!result.success_);
    QVERIFY(result.fieldErrors_.contains(QStringLiteral("jobTitle")));
    QVERIFY(result.fieldErrors_.contains(QStringLiteral("jobUrl")));
    QVERIFY(result.fieldErrors_.contains(QStringLiteral("cv")));
    QVERIFY(jobs.findAll().isEmpty());
    QVERIFY(cvs.findAll().isEmpty());
    QVERIFY(QDir(paths.resumesDirectory()).entryList(QDir::Files).isEmpty());
}

void StorageAndAddJobTest::controllerPublishesSuccessfulCreation()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SqliteDatabase database(paths.databasePath());
    CvRepository cvs(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, importer);
    JobApplicationsController controller({}, service);
    QSignalSpy createdSpy(&controller, &JobApplicationsController::applicationCreated);
    QSignalSpy failedSpy(&controller, &JobApplicationsController::saveFailed);
    const auto sourcePath = createCvFile(temporaryDirectory.path());

    controller.createApplication(
        {
            {QStringLiteral("jobTitle"), QStringLiteral("Qt Developer")},
            {QStringLiteral("companyName"), QStringLiteral("Example Company")},
            {QStringLiteral("status"), QStringLiteral("Applied")},
            {QStringLiteral("appliedDate"), QStringLiteral("2026-07-09")},
        },
        QUrl::fromLocalFile(sourcePath));

    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(createdSpy.count(), 1);
    QCOMPARE(controller.applicationCount(), 1);
    QVERIFY(!controller.saving());
}

QTEST_GUILESS_MAIN(StorageAndAddJobTest)

#include "StorageAndAddJobTest.moc"

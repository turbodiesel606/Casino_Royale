#include "cvs/CvImportService.hpp"
#include "cvs/CvRepository.hpp"
#include "directory/CompanyListModel.hpp"
#include "directory/CompanyDirectoryController.hpp"
#include "directory/CompanyRepository.hpp"
#include "directory/ContactListModel.hpp"
#include "jobs/AddJobService.hpp"
#include "jobs/JobApplicationDraft.hpp"
#include "jobs/JobApplicationsController.hpp"
#include "jobs/JobApplicationListModel.hpp"
#include "jobs/JobRepository.hpp"
#include "storage/SqliteDatabase.hpp"
#include "storage/SchemaMigrator.hpp"
#include "storage/StoragePaths.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest/QtTest>

namespace {

QString createCvFile(
    const QString& directory,
    const QString& name = QStringLiteral("resume.pdf"),
    const QByteArray& contents = QByteArrayLiteral("%PDF-1.4 JobTracker test CV"))
{
    const auto path = QDir(directory).filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    file.write(contents);
    return path;
}

bool createVersionOneDatabase(const QString& databasePath, QString& errorMessage)
{
    const auto connectionName = QStringLiteral("jobtracker-v1-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(databasePath);

    bool succeeded = database.open();
    const QStringList statements{
        QStringLiteral("PRAGMA foreign_keys = ON"),
        QStringLiteral(
            "CREATE TABLE cvs ("
            "id TEXT PRIMARY KEY,"
            "original_file_name TEXT NOT NULL,"
            "stored_file_name TEXT NOT NULL,"
            "relative_path TEXT NOT NULL UNIQUE,"
            "sha256 TEXT NOT NULL UNIQUE,"
            "size_bytes INTEGER NOT NULL,"
            "title TEXT NOT NULL DEFAULT '',"
            "category TEXT NOT NULL DEFAULT '',"
            "language TEXT NOT NULL DEFAULT '',"
            "description TEXT NOT NULL DEFAULT '',"
            "is_favorite INTEGER NOT NULL DEFAULT 0,"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE jobs ("
            "id TEXT PRIMARY KEY,"
            "company_name TEXT NOT NULL,"
            "job_title TEXT NOT NULL,"
            "job_url TEXT NOT NULL DEFAULT '',"
            "work_format TEXT NOT NULL DEFAULT '',"
            "city TEXT NOT NULL DEFAULT '',"
            "salary TEXT NOT NULL DEFAULT '',"
            "status TEXT NOT NULL,"
            "applied_date TEXT NOT NULL,"
            "next_step TEXT NOT NULL DEFAULT '',"
            "cv_id TEXT NOT NULL,"
            "description TEXT NOT NULL DEFAULT '',"
            "requirements TEXT NOT NULL DEFAULT '',"
            "notes TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL,"
            "FOREIGN KEY (cv_id) REFERENCES cvs(id))"),
        QStringLiteral(
            "CREATE TABLE job_technologies ("
            "job_id TEXT NOT NULL,"
            "position INTEGER NOT NULL,"
            "technology TEXT NOT NULL,"
            "PRIMARY KEY (job_id, position),"
            "FOREIGN KEY (job_id) REFERENCES jobs(id) ON DELETE CASCADE)"),
        QStringLiteral(
            "INSERT INTO cvs VALUES ("
            "'legacy-cv', 'resume.pdf', 'legacy.pdf', 'Resumes/legacy.pdf', "
            "'legacy-hash', 123, 'Legacy CV', 'General', 'English', '', 0, "
            "'2026-07-01T10:00:00Z', '2026-07-01T10:00:00Z')"),
        QStringLiteral(
            "INSERT INTO jobs VALUES ("
            "'legacy-job', 'Legacy Company', 'Legacy Role', '', '', '', '', "
            "'Applied', '2026-07-01', '', 'legacy-cv', '', '', '', "
            "'2026-07-01T10:00:00Z', '2026-07-01T10:00:00Z')"),
        QStringLiteral(
            "INSERT INTO job_technologies VALUES ('legacy-job', 0, 'Qt')"),
        QStringLiteral("PRAGMA user_version = 1"),
    };

    if (!succeeded) {
        errorMessage = database.lastError().text();
    }

    {
        QSqlQuery query{database};
        for (const auto& statement : statements) {
            if (!succeeded) {
                break;
            }
            succeeded = query.exec(statement);
            if (!succeeded) {
                errorMessage = query.lastError().text();
            }
        }
    }

    database.close();
    database = {};
    QSqlDatabase::removeDatabase(connectionName);
    return succeeded;
}

bool createVersionTwoDatabase(
    const QString& databasePath,
    const QStringList& companyNames,
    QString& errorMessage)
{
    const auto connectionName = QStringLiteral("jobtracker-v2-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(databasePath);

    bool succeeded = database.open();
    const QStringList statements{
        QStringLiteral("PRAGMA foreign_keys = ON"),
        QStringLiteral(
            "CREATE TABLE cvs ("
            "id TEXT PRIMARY KEY,"
            "original_file_name TEXT NOT NULL,"
            "stored_file_name TEXT NOT NULL,"
            "relative_path TEXT NOT NULL UNIQUE,"
            "sha256 TEXT NOT NULL,"
            "size_bytes INTEGER NOT NULL,"
            "title TEXT NOT NULL DEFAULT '',"
            "category TEXT NOT NULL DEFAULT '',"
            "language TEXT NOT NULL DEFAULT '',"
            "description TEXT NOT NULL DEFAULT '',"
            "is_favorite INTEGER NOT NULL DEFAULT 0,"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL,"
            "UNIQUE (sha256, original_file_name))"),
        QStringLiteral(
            "CREATE TABLE jobs ("
            "id TEXT PRIMARY KEY,"
            "company_name TEXT NOT NULL,"
            "job_title TEXT NOT NULL,"
            "job_url TEXT NOT NULL DEFAULT '',"
            "work_format TEXT NOT NULL DEFAULT '',"
            "city TEXT NOT NULL DEFAULT '',"
            "salary TEXT NOT NULL DEFAULT '',"
            "status TEXT NOT NULL,"
            "applied_date TEXT NOT NULL,"
            "next_step TEXT NOT NULL DEFAULT '',"
            "cv_id TEXT NOT NULL,"
            "description TEXT NOT NULL DEFAULT '',"
            "requirements TEXT NOT NULL DEFAULT '',"
            "notes TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL,"
            "FOREIGN KEY (cv_id) REFERENCES cvs(id))"),
        QStringLiteral(
            "CREATE TABLE job_technologies ("
            "job_id TEXT NOT NULL,"
            "position INTEGER NOT NULL,"
            "technology TEXT NOT NULL,"
            "PRIMARY KEY (job_id, position),"
            "FOREIGN KEY (job_id) REFERENCES jobs(id) ON DELETE CASCADE)"),
        QStringLiteral(
            "INSERT INTO cvs VALUES ("
            "'legacy-cv', 'resume.pdf', 'legacy.pdf', 'Resumes/legacy.pdf', "
            "'legacy-hash', 123, 'Legacy CV', 'General', 'English', '', 0, "
            "'2026-07-01T10:00:00Z', '2026-07-01T10:00:00Z')"),
    };

    if (!succeeded) {
        errorMessage = database.lastError().text();
    }

    {
        QSqlQuery query{database};
        for (const auto& statement : statements) {
            if (!succeeded) {
                break;
            }
            succeeded = query.exec(statement);
            if (!succeeded) {
                errorMessage = query.lastError().text();
            }
        }

        if (succeeded) {
            query.prepare(QStringLiteral(
                "INSERT INTO jobs ("
                "id, company_name, job_title, status, applied_date, cv_id, created_at, updated_at) "
                "VALUES (?, ?, ?, 'Applied', '2026-07-01', 'legacy-cv', ?, ?)"));
            for (int index = 0; index < companyNames.size(); ++index) {
                const auto timestamp = QStringLiteral("2026-07-01T10:00:%1Z")
                    .arg(index, 2, 10, QLatin1Char('0'));
                query.bindValue(0, QStringLiteral("legacy-job-%1").arg(index));
                query.bindValue(1, companyNames.at(index));
                query.bindValue(2, QStringLiteral("Legacy Role %1").arg(index));
                query.bindValue(3, timestamp);
                query.bindValue(4, timestamp);
                if (!query.exec()) {
                    succeeded = false;
                    errorMessage = query.lastError().text();
                    break;
                }
            }
        }

        if (succeeded) {
            succeeded = query.exec(QStringLiteral("PRAGMA user_version = 2"));
            if (!succeeded) {
                errorMessage = query.lastError().text();
            }
        }
    }

    database.close();
    database = {};
    QSqlDatabase::removeDatabase(connectionName);
    return succeeded;
}

JobApplicationDraft validDraft()
{
    JobApplicationDraft draft;
    draft.jobTitle_ = QStringLiteral("Qt Developer");
    draft.companyName_ = QStringLiteral("Example Company");
    draft.jobUrl_ = QStringLiteral("https://example.com/jobs/qt");
    draft.workFormat_ = QStringLiteral("Remote");
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
    void reusesCvWithSameIdentity();
    void createsSeparateCvForSameContentWithDifferentName();
    void createsSeparateCvForDifferentContentWithSameName();
    void migratesVersionOneAndPreservesLinks();
    void migratesVersionTwoCompanyIdentity();
    void rejectsBlankLegacyCompanyMigrationTransactionally();
    void persistsJobAcrossDatabaseReopen();
    void persistsFavoriteAcrossDatabaseReopen();
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
        SQLiteDataBase database(paths.databasePath());
        QSqlQuery version(database.connection());
        QVERIFY(version.exec(QStringLiteral("PRAGMA user_version")));
        QVERIFY(version.next());
        QCOMPARE(version.value(0).toInt(), 3);

        QSqlQuery foreignKeys(database.connection());
        QVERIFY(foreignKeys.exec(QStringLiteral("PRAGMA foreign_keys")));
        QVERIFY(foreignKeys.next());
        QCOMPARE(foreignKeys.value(0).toInt(), 1);
    }

    SQLiteDataBase reopened(paths.databasePath());
    CompanyRepository companies(reopened.connection());
    JobRepository jobs(reopened.connection());
    CvRepository cvs(reopened.connection());
    QVERIFY(companies.findAll().isEmpty());
    QVERIFY(jobs.findAll().isEmpty());
    QVERIFY(cvs.findAll().isEmpty());
}

void StorageAndAddJobTest::createsJobAndCopiesCv()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SQLiteDataBase database(paths.databasePath());
    CvRepository cvs(database.connection());
    CompanyRepository companies(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, companies, importer);
    const auto sourcePath = createCvFile(temporaryDirectory.path());

    const auto result = service.create(validDraft(), QUrl::fromLocalFile(sourcePath));

    QVERIFY2(result.success_, qPrintable(result.message_));
    QVERIFY(result.cvWasInserted_);
    QVERIFY(!result.company_.id_.isEmpty());
    QCOMPARE(result.application_.companyId_, result.company_.id_);
    const auto storedApplications = jobs.findAll();
    const auto storedCvs = cvs.findAll();
    const auto storedCompanies = companies.findAll();
    QCOMPARE(storedApplications.size(), 1);
    QCOMPARE(storedApplications.first().techStack_, QStringList({QStringLiteral("Qt"), QStringLiteral("C++")}));
    QCOMPARE(storedApplications.first().jobUrl_, QUrl{QStringLiteral("https://example.com/jobs/qt")});
    QCOMPARE(storedApplications.first().workFormat_, WorkFormat::Remote);
    QCOMPARE(storedApplications.first().status_, JobStatus::Applied);
    QCOMPARE(storedApplications.first().appliedDate_, QDate(2026, 7, 9));
    QVERIFY(storedApplications.first().createdAt_.isValid());
    QVERIFY(storedApplications.first().updatedAt_.isValid());
    QCOMPARE(storedCvs.size(), 1);
    QVERIFY(storedCvs.first().createdAt_.isValid());
    QVERIFY(storedCvs.first().updatedAt_.isValid());
    QCOMPARE(storedCompanies.size(), 1);
    QVERIFY(storedCompanies.first().createdAt_.isValid());
    QVERIFY(storedCompanies.first().updatedAt_.isValid());

    JobApplicationListModel applicationModel{storedApplications};
    const auto applicationIndex = applicationModel.index(0, 0);
    QCOMPARE(
        applicationModel.data(applicationIndex, JobApplicationListModel::JobUrlRole).toString(),
        QStringLiteral("https://example.com/jobs/qt"));
    QCOMPARE(
        applicationModel.data(applicationIndex, JobApplicationListModel::WorkFormatRole).toString(),
        QStringLiteral("Remote"));
    QCOMPARE(
        applicationModel.data(applicationIndex, JobApplicationListModel::StatusLabelRole).toString(),
        QStringLiteral("Applied"));
    QCOMPARE(
        applicationModel.data(applicationIndex, JobApplicationListModel::AppliedDateRole).toString(),
        QStringLiteral("2026-07-09"));
    QCOMPARE(
        applicationModel.data(applicationIndex, JobApplicationListModel::DateLabelRole).toString(),
        QStringLiteral("Jul 9, 2026"));
    QCOMPARE(
        applicationModel.data(applicationIndex, JobApplicationListModel::CreatedAtRole).toDateTime(),
        storedApplications.first().createdAt_);

    CompanyListModel companyModel{storedCompanies};
    const auto companyIndex = companyModel.index(0, 0);
    QCOMPARE(
        companyModel.data(companyIndex, CompanyListModel::LogoTextRole).toString(),
        QStringLiteral("EX"));
    QCOMPARE(
        companyModel.data(companyIndex, CompanyListModel::CreatedAtRole).toDateTime(),
        storedCompanies.first().createdAt_);
    QVERIFY(QFileInfo::exists(QDir(paths.dataDirectory()).filePath(result.cvDocument_.relativePath_)));

    QSqlQuery invalidCompany(database.connection());
    invalidCompany.prepare(QStringLiteral("UPDATE jobs SET company_id = ? WHERE id = ?"));
    invalidCompany.addBindValue(QStringLiteral("missing-company"));
    invalidCompany.addBindValue(result.application_.id_);
    QVERIFY(!invalidCompany.exec());
}

void StorageAndAddJobTest::reusesCvWithSameIdentity()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SQLiteDataBase database(paths.databasePath());
    CvRepository cvs(database.connection());
    CompanyRepository companies(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, companies, importer);
    const auto sourcePath = createCvFile(temporaryDirectory.path());

    const auto first = service.create(validDraft(), QUrl::fromLocalFile(sourcePath));
    auto secondDraft = validDraft();
    secondDraft.jobTitle_ = QStringLiteral("Senior Qt Developer");
    secondDraft.companyName_ = QStringLiteral("  example COMPANY  ");
    const auto second = service.create(secondDraft, QUrl::fromLocalFile(sourcePath));

    QVERIFY(first.success_);
    QVERIFY(second.success_);
    QVERIFY(!second.cvWasInserted_);
    QCOMPARE(first.cvDocument_.id_, second.cvDocument_.id_);
    QCOMPARE(first.application_.companyId_, second.application_.companyId_);
    QCOMPARE(second.application_.companyName_, QStringLiteral("Example Company"));
    QCOMPARE(companies.findAll().size(), 1);
    QCOMPARE(cvs.findAll().size(), 1);
    QCOMPARE(jobs.findAll().size(), 2);
    QCOMPARE(QDir(paths.resumesDirectory()).entryList(QDir::Files).size(), 1);
}

void StorageAndAddJobTest::createsSeparateCvForSameContentWithDifferentName()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SQLiteDataBase database(paths.databasePath());
    CvRepository cvs(database.connection());
    CompanyRepository companies(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, companies, importer);
    const auto firstPath = createCvFile(temporaryDirectory.path(), QStringLiteral("first.pdf"));
    const auto secondPath = createCvFile(temporaryDirectory.path(), QStringLiteral("second.pdf"));

    const auto first = service.create(validDraft(), QUrl::fromLocalFile(firstPath));
    auto secondDraft = validDraft();
    secondDraft.jobTitle_ = QStringLiteral("Senior Qt Developer");
    const auto second = service.create(secondDraft, QUrl::fromLocalFile(secondPath));

    QVERIFY(first.success_);
    QVERIFY(second.success_);
    QVERIFY(second.cvWasInserted_);
    QVERIFY(first.cvDocument_.id_ != second.cvDocument_.id_);
    QCOMPARE(cvs.findAll().size(), 2);
    QCOMPARE(jobs.findAll().size(), 2);
    QCOMPARE(QDir(paths.resumesDirectory()).entryList(QDir::Files).size(), 2);
}

void StorageAndAddJobTest::createsSeparateCvForDifferentContentWithSameName()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SQLiteDataBase database(paths.databasePath());
    CvRepository cvs(database.connection());
    CompanyRepository companies(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, companies, importer);
    const auto firstSourceDirectory = QDir(temporaryDirectory.path()).filePath(QStringLiteral("first"));
    const auto secondSourceDirectory = QDir(temporaryDirectory.path()).filePath(QStringLiteral("second"));
    QVERIFY(QDir().mkpath(firstSourceDirectory));
    QVERIFY(QDir().mkpath(secondSourceDirectory));
    const auto firstPath = createCvFile(
        firstSourceDirectory,
        QStringLiteral("resume.pdf"),
        QByteArrayLiteral("%PDF-1.4 First CV"));
    const auto secondPath = createCvFile(
        secondSourceDirectory,
        QStringLiteral("resume.pdf"),
        QByteArrayLiteral("%PDF-1.4 Second CV"));

    const auto first = service.create(validDraft(), QUrl::fromLocalFile(firstPath));
    auto secondDraft = validDraft();
    secondDraft.jobTitle_ = QStringLiteral("Senior Qt Developer");
    const auto second = service.create(secondDraft, QUrl::fromLocalFile(secondPath));

    QVERIFY(first.success_);
    QVERIFY(second.success_);
    QVERIFY(second.cvWasInserted_);
    QVERIFY(first.cvDocument_.id_ != second.cvDocument_.id_);
    QCOMPARE(cvs.findAll().size(), 2);
    QCOMPARE(jobs.findAll().size(), 2);
    QCOMPARE(QDir(paths.resumesDirectory()).entryList(QDir::Files).size(), 2);
}

void StorageAndAddJobTest::migratesVersionOneAndPreservesLinks()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    QString setupError;
    QVERIFY2(createVersionOneDatabase(paths.databasePath(), setupError), qPrintable(setupError));

    SQLiteDataBase database(paths.databasePath());
    QSqlQuery version(database.connection());
    QVERIFY(version.exec(QStringLiteral("PRAGMA user_version")));
    QVERIFY(version.next());
    QCOMPARE(version.value(0).toInt(), 3);

    CvRepository cvs(database.connection());
    CompanyRepository companies(database.connection());
    JobRepository jobs(database.connection());
    const auto documents = cvs.findAll();
    const auto storedCompanies = companies.findAll();
    const auto applications = jobs.findAll();
    QCOMPARE(documents.size(), 1);
    QCOMPARE(documents.first().id_, QStringLiteral("legacy-cv"));
    QCOMPARE(documents.first().linkedApplicationIds_, QStringList{QStringLiteral("legacy-job")});
    QCOMPARE(storedCompanies.size(), 1);
    QCOMPARE(storedCompanies.first().name_, QStringLiteral("Legacy Company"));
    QCOMPARE(applications.size(), 1);
    QCOMPARE(applications.first().id_, QStringLiteral("legacy-job"));
    QCOMPARE(applications.first().companyId_, storedCompanies.first().id_);
    QCOMPARE(applications.first().companyName_, QStringLiteral("Legacy Company"));
    QCOMPARE(applications.first().cvId_, QStringLiteral("legacy-cv"));
    QCOMPARE(applications.first().techStack_, QStringList{QStringLiteral("Qt")});

    QSqlQuery foreignKeyCheck(database.connection());
    QVERIFY(foreignKeyCheck.exec(QStringLiteral("PRAGMA foreign_key_check")));
    QVERIFY(!foreignKeyCheck.next());
}

void StorageAndAddJobTest::migratesVersionTwoCompanyIdentity()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    QString setupError;
    QVERIFY2(
        createVersionTwoDatabase(
            paths.databasePath(),
            {QStringLiteral("  Acme  "), QStringLiteral("acme")},
            setupError),
        qPrintable(setupError));

    SQLiteDataBase database(paths.databasePath());
    QSqlQuery version(database.connection());
    QVERIFY(version.exec(QStringLiteral("PRAGMA user_version")));
    QVERIFY(version.next());
    QCOMPARE(version.value(0).toInt(), 3);

    CompanyRepository companies(database.connection());
    JobRepository jobs(database.connection());
    const auto storedCompanies = companies.findAll();
    const auto applications = jobs.findAll();
    QCOMPARE(storedCompanies.size(), 1);
    QCOMPARE(storedCompanies.first().name_, QStringLiteral("Acme"));
    QCOMPARE(applications.size(), 2);
    QCOMPARE(applications.at(0).companyId_, storedCompanies.first().id_);
    QCOMPARE(applications.at(1).companyId_, storedCompanies.first().id_);
    QCOMPARE(applications.at(0).companyName_, QStringLiteral("Acme"));
    QCOMPARE(applications.at(1).companyName_, QStringLiteral("Acme"));

    QSqlQuery normalizedName(database.connection());
    QVERIFY(normalizedName.exec(QStringLiteral("SELECT normalized_name FROM companies")));
    QVERIFY(normalizedName.next());
    QCOMPARE(normalizedName.value(0).toString(), QStringLiteral("acme"));

    bool hasCompanyForeignKey = false;
    QSqlQuery foreignKeys(database.connection());
    QVERIFY(foreignKeys.exec(QStringLiteral("PRAGMA foreign_key_list(jobs)")));
    while (foreignKeys.next()) {
        if (foreignKeys.value(QStringLiteral("table")).toString() == QStringLiteral("companies")
            && foreignKeys.value(QStringLiteral("from")).toString() == QStringLiteral("company_id")) {
            hasCompanyForeignKey = true;
        }
    }
    QVERIFY(hasCompanyForeignKey);
}

void StorageAndAddJobTest::rejectsBlankLegacyCompanyMigrationTransactionally()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    QString setupError;
    QVERIFY2(
        createVersionTwoDatabase(
            paths.databasePath(),
            {QStringLiteral("  \t  ")},
            setupError),
        qPrintable(setupError));

    const auto connectionName = QStringLiteral("jobtracker-invalid-migration-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(paths.databasePath());
    QVERIFY(database.open());
    QSqlQuery pragmas(database);
    QVERIFY(pragmas.exec(QStringLiteral("PRAGMA foreign_keys = ON")));

    QString migrationError;
    try {
        SchemaMigrator::migrate(database);
    } catch (const std::exception& error) {
        migrationError = QString::fromUtf8(error.what());
    }
    QVERIFY(migrationError.contains(QStringLiteral("blank company name")));

    QSqlQuery version(database);
    QVERIFY(version.exec(QStringLiteral("PRAGMA user_version")));
    QVERIFY(version.next());
    QCOMPARE(version.value(0).toInt(), 2);

    QSqlQuery companiesTable(database);
    QVERIFY(companiesTable.exec(QStringLiteral(
        "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'companies'")));
    QVERIFY(companiesTable.next());
    QCOMPARE(companiesTable.value(0).toInt(), 0);

    QSqlQuery jobs(database);
    QVERIFY(jobs.exec(QStringLiteral("SELECT company_name FROM jobs")));
    QVERIFY(jobs.next());
    QCOMPARE(jobs.value(0).toString(), QStringLiteral("  \t  "));

    database.close();
    database = {};
    QSqlDatabase::removeDatabase(connectionName);
}

void StorageAndAddJobTest::persistsJobAcrossDatabaseReopen()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    const auto sourcePath = createCvFile(temporaryDirectory.path());
    QString companyId;

    {
        SQLiteDataBase database(paths.databasePath());
        CvRepository cvs(database.connection());
        CompanyRepository companies(database.connection());
        JobRepository jobs(database.connection());
        CvImportService importer(paths, cvs);
        AddJobService service(database.connection(), jobs, companies, importer);
        const auto result = service.create(validDraft(), QUrl::fromLocalFile(sourcePath));
        QVERIFY(result.success_);
        companyId = result.company_.id_;
    }

    SQLiteDataBase reopened(paths.databasePath());
    CvRepository cvs(reopened.connection());
    CompanyRepository companies(reopened.connection());
    JobRepository jobs(reopened.connection());
    const auto storedCompanies = companies.findAll();
    const auto applications = jobs.findAll();
    QCOMPARE(storedCompanies.size(), 1);
    QCOMPARE(storedCompanies.first().id_, companyId);
    QCOMPARE(storedCompanies.first().name_, QStringLiteral("Example Company"));
    QCOMPARE(applications.size(), 1);
    QCOMPARE(applications.first().companyId_, companyId);
    QCOMPARE(cvs.findAll().size(), 1);
    QCOMPARE(applications.first().jobTitle_, QStringLiteral("Qt Developer"));

    JobApplicationListModel applicationModel(applications);
    ContactListModel contacts;
    CompanyDirectoryController companyDirectory(
        storedCompanies,
        applicationModel,
        contacts);
    QCOMPARE(companyDirectory.companyCount(), 1);
    QCOMPARE(companyDirectory.selectedCompanyId(), companyId);
    QCOMPARE(companyDirectory.linkedJobsModel()->rowCount(), 1);
}

void StorageAndAddJobTest::persistsFavoriteAcrossDatabaseReopen()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    const auto sourcePath = createCvFile(temporaryDirectory.path());
    QString cvId;
    QDateTime originalUpdatedAt;

    {
        SQLiteDataBase database(paths.databasePath());
        CvRepository cvs(database.connection());
        CompanyRepository companies(database.connection());
        JobRepository jobs(database.connection());
        CvImportService importer(paths, cvs);
        AddJobService service(database.connection(), jobs, companies, importer);
        const auto result = service.create(validDraft(), QUrl::fromLocalFile(sourcePath));
        QVERIFY(result.success_);
        cvId = result.cvDocument_.id_;
        originalUpdatedAt = result.cvDocument_.updatedAt_;

        const auto updatedAt = cvs.updateFavorite(cvId, true);
        QVERIFY(updatedAt.has_value());
        const auto updated = cvs.findAll();
        QCOMPARE(updated.size(), 1);
        QVERIFY(updated.first().isFavorite_);
        QVERIFY(updated.first().updatedAt_ != originalUpdatedAt);
    }

    SQLiteDataBase reopened(paths.databasePath());
    CvRepository cvs(reopened.connection());
    const auto persisted = cvs.findAll();
    QCOMPARE(persisted.size(), 1);
    QCOMPARE(persisted.first().id_, cvId);
    QVERIFY(persisted.first().isFavorite_);
    QVERIFY(persisted.first().updatedAt_ != originalUpdatedAt);
}

void StorageAndAddJobTest::removesCopiedCvWhenJobInsertFails()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SQLiteDataBase database(paths.databasePath());
    CvRepository cvs(database.connection());
    CompanyRepository companies(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, companies, importer);
    QSqlQuery trigger(database.connection());
    QVERIFY(trigger.exec(QStringLiteral(
        "CREATE TRIGGER reject_job BEFORE INSERT ON jobs "
        "BEGIN SELECT RAISE(FAIL, 'forced job failure'); END")));

    const auto result = service.create(
        validDraft(),
        QUrl::fromLocalFile(createCvFile(temporaryDirectory.path())));

    QVERIFY(!result.success_);
    QVERIFY(jobs.findAll().isEmpty());
    QVERIFY(companies.findAll().isEmpty());
    QVERIFY(cvs.findAll().isEmpty());
    QVERIFY(QDir(paths.resumesDirectory()).entryList(QDir::Files).isEmpty());
}

void StorageAndAddJobTest::rejectsInvalidInputWithoutWriting()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SQLiteDataBase database(paths.databasePath());
    CvRepository cvs(database.connection());
    CompanyRepository companies(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, companies, importer);
    auto draft = validDraft();
    draft.jobTitle_.clear();
    draft.jobUrl_ = QStringLiteral("file:///not-a-job");

    const auto result = service.create(draft, {});

    QVERIFY(!result.success_);
    QVERIFY(result.fieldErrors_.contains(QStringLiteral("jobTitle")));
    QVERIFY(result.fieldErrors_.contains(QStringLiteral("jobUrl")));
    QVERIFY(result.fieldErrors_.contains(QStringLiteral("cv")));
    QVERIFY(jobs.findAll().isEmpty());
    QVERIFY(companies.findAll().isEmpty());
    QVERIFY(cvs.findAll().isEmpty());
    QVERIFY(QDir(paths.resumesDirectory()).entryList(QDir::Files).isEmpty());
}

void StorageAndAddJobTest::controllerPublishesSuccessfulCreation()
{
    QTemporaryDir temporaryDirectory;
    StoragePaths paths(QDir(temporaryDirectory.path()).filePath(QStringLiteral("Data")));
    SQLiteDataBase database(paths.databasePath());
    CvRepository cvs(database.connection());
    CompanyRepository companies(database.connection());
    JobRepository jobs(database.connection());
    CvImportService importer(paths, cvs);
    AddJobService service(database.connection(), jobs, companies, importer);
    JobApplicationsController controller({}, service);
    ContactListModel contacts;
    CompanyDirectoryController companyDirectory(
        companies.findAll(),
        controller.jobApplicationListModel(),
        contacts);
    QObject::connect(
        &controller,
        &JobApplicationsController::companyResolved,
        &companyDirectory,
        &CompanyDirectoryController::publishCompany);
    QSignalSpy createdSpy(&controller, &JobApplicationsController::applicationCreated);
    QSignalSpy companySpy(&controller, &JobApplicationsController::companyResolved);
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

QTEST_GUILESS_MAIN(StorageAndAddJobTest)

#include "StorageAndAddJobTest.moc"

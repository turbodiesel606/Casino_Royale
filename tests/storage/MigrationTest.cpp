#include "cvs/CvRepository.hpp"
#include "directory/CompanyRepository.hpp"
#include "jobs/JobRepository.hpp"
#include "storage/SchemaMigrator.hpp"

#include "../support/StorageTestFixtures.hpp"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QtTest/QtTest>

#include <exception>

namespace {

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
        QStringLiteral("INSERT INTO job_technologies VALUES ('legacy-job', 0, 'Qt')"),
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
                    .arg(index, 2, 10, QLatin1Char{'0'});
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

bool createVersionThreeDatabase(const QString& databasePath, QString& errorMessage)
{
    const auto connectionName = QStringLiteral("jobtracker-v3-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(databasePath);

    bool succeeded = database.open();
    const QStringList statements{
        QStringLiteral("PRAGMA foreign_keys = ON"),
        QStringLiteral(
            "CREATE TABLE cvs ("
            "id TEXT PRIMARY KEY, original_file_name TEXT NOT NULL, stored_file_name TEXT NOT NULL, "
            "relative_path TEXT NOT NULL UNIQUE, sha256 TEXT NOT NULL, size_bytes INTEGER NOT NULL, "
            "title TEXT NOT NULL DEFAULT '', category TEXT NOT NULL DEFAULT '', "
            "language TEXT NOT NULL DEFAULT '', description TEXT NOT NULL DEFAULT '', "
            "is_favorite INTEGER NOT NULL DEFAULT 0, created_at TEXT NOT NULL, updated_at TEXT NOT NULL, "
            "UNIQUE (sha256, original_file_name))"),
        QStringLiteral(
            "CREATE TABLE companies ("
            "id TEXT PRIMARY KEY, display_name TEXT NOT NULL, normalized_name TEXT NOT NULL UNIQUE, "
            "created_at TEXT NOT NULL, updated_at TEXT NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE jobs ("
            "id TEXT PRIMARY KEY, company_id TEXT NOT NULL, job_title TEXT NOT NULL, "
            "job_url TEXT NOT NULL DEFAULT '', work_format TEXT NOT NULL DEFAULT '', "
            "city TEXT NOT NULL DEFAULT '', salary TEXT NOT NULL DEFAULT '', status TEXT NOT NULL, "
            "applied_date TEXT NOT NULL, next_step TEXT NOT NULL DEFAULT '', cv_id TEXT NOT NULL, "
            "description TEXT NOT NULL DEFAULT '', requirements TEXT NOT NULL DEFAULT '', "
            "notes TEXT NOT NULL DEFAULT '', created_at TEXT NOT NULL, updated_at TEXT NOT NULL, "
            "FOREIGN KEY (company_id) REFERENCES companies(id), FOREIGN KEY (cv_id) REFERENCES cvs(id))"),
        QStringLiteral(
            "CREATE TABLE job_technologies ("
            "job_id TEXT NOT NULL, position INTEGER NOT NULL, technology TEXT NOT NULL, "
            "PRIMARY KEY (job_id, position), "
            "FOREIGN KEY (job_id) REFERENCES jobs(id) ON DELETE CASCADE)"),
        QStringLiteral(
            "INSERT INTO cvs VALUES ('v3-cv', 'resume.pdf', 'v3.pdf', 'Resumes/v3.pdf', "
            "'v3-hash', 42, 'V3 CV', 'General', 'English', '', 1, "
            "'2026-07-01T10:00:00Z', '2026-07-01T10:00:00Z')"),
        QStringLiteral(
            "INSERT INTO companies VALUES ('v3-company', 'V3 Company', 'v3 company', "
            "'2026-07-01T10:00:00Z', '2026-07-01T10:00:00Z')"),
        QStringLiteral(
            "INSERT INTO jobs (id, company_id, job_title, status, applied_date, cv_id, created_at, updated_at) "
            "VALUES ('v3-job', 'v3-company', 'V3 Role', 'Applied', '2026-07-01', 'v3-cv', "
            "'2026-07-01T10:00:00Z', '2026-07-01T10:00:00Z')"),
        QStringLiteral("PRAGMA user_version = 3"),
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

} // namespace

class MigrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void initializesCurrentSchemaAndReopens();
    void rejectsNewerSchemaVersion();
    void constructorFailureUnregistersNamedConnection();
    void migratesVersionOneAndPreservesLinks();
    void migratesVersionTwoCompanyIdentity();
    void migratesVersionThreeArchiveState();
    void rejectsBlankLegacyCompanyTransactionally();
};

void MigrationTest::initializesCurrentSchemaAndReopens()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());

    {
        SqliteDatabase database{storage.paths().databasePath()};
        QSqlQuery version{database.connection()};
        QVERIFY(version.exec(QStringLiteral("PRAGMA user_version")));
        QVERIFY(version.next());
        QCOMPARE(version.value(0).toInt(), 4);

        QSqlQuery foreignKeys{database.connection()};
        QVERIFY(foreignKeys.exec(QStringLiteral("PRAGMA foreign_keys")));
        QVERIFY(foreignKeys.next());
        QCOMPARE(foreignKeys.value(0).toInt(), 1);

        QSqlQuery columns{database.connection()};
        QVERIFY(columns.exec(QStringLiteral("PRAGMA table_info(cvs)")));
        bool foundArchivedAt = false;
        while (columns.next()) {
            if (columns.value(1).toString() == QStringLiteral("archived_at")) {
                foundArchivedAt = true;
                break;
            }
        }
        QVERIFY(foundArchivedAt);
    }

    SqliteDatabase reopened{storage.paths().databasePath()};
    CompanyRepository companies{reopened.connection()};
    JobRepository jobs{reopened.connection()};
    CvRepository cvs{reopened.connection()};
    QVERIFY(companies.findAll().isEmpty());
    QVERIFY(jobs.findAll().isEmpty());
    QVERIFY(cvs.findAll().isEmpty());
}

void MigrationTest::rejectsNewerSchemaVersion()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    const auto databasePath = QDir{storage.rootPath()}.filePath(QStringLiteral("newer-schema.sqlite"));
    const auto connectionName = QStringLiteral("jobtracker-newer-schema-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(databasePath);
    QVERIFY(database.open());
    {
        QSqlQuery version{database};
        QVERIFY(version.exec(QStringLiteral("PRAGMA user_version = 5")));
    }

    QString migrationError;
    try {
        SchemaMigrator::migrate(database);
    } catch (const std::exception& error) {
        migrationError = QString::fromUtf8(error.what());
    }
    QVERIFY(migrationError.contains(QStringLiteral("newer")));

    database.close();
    database = {};
    QSqlDatabase::removeDatabase(connectionName);
}

void MigrationTest::constructorFailureUnregistersNamedConnection()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    const auto databasePath = storage.paths().databasePath();
    const auto setupConnectionName = QStringLiteral("jobtracker-constructor-failure-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto setupDatabase = QSqlDatabase::addDatabase(
        QStringLiteral("QSQLITE"),
        setupConnectionName);
    setupDatabase.setDatabaseName(databasePath);
    QVERIFY(setupDatabase.open());
    {
        QSqlQuery version{setupDatabase};
        QVERIFY(version.exec(QStringLiteral("PRAGMA user_version = 5")));
    }
    setupDatabase.close();
    setupDatabase = {};
    QSqlDatabase::removeDatabase(setupConnectionName);

    const auto initialConnectionCount = QSqlDatabase::connectionNames().size();
    QString constructionError;
    try {
        SqliteDatabase database{databasePath};
    }
    catch (const std::exception& error) {
        constructionError = QString::fromUtf8(error.what());
    }

    QVERIFY(constructionError.contains(QStringLiteral("newer")));
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);
}

void MigrationTest::migratesVersionOneAndPreservesLinks()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    QString setupError;
    QVERIFY2(
        createVersionOneDatabase(storage.paths().databasePath(), setupError),
        qPrintable(setupError));

    SqliteDatabase database{storage.paths().databasePath()};
    CvRepository cvs{database.connection()};
    CompanyRepository companies{database.connection()};
    JobRepository jobs{database.connection()};
    const auto documents = cvs.findAll();
    const auto storedCompanies = companies.findAll();
    const auto applications = jobs.findAll();

    QCOMPARE(documents.size(), 1);
    QCOMPARE(documents.first().id_, QStringLiteral("legacy-cv"));
    QVERIFY(!documents.first().archivedAt_.isValid());
    QCOMPARE(documents.first().linkedApplicationIds_, QStringList{QStringLiteral("legacy-job")});
    QCOMPARE(storedCompanies.size(), 1);
    QCOMPARE(storedCompanies.first().name_, QStringLiteral("Legacy Company"));
    QCOMPARE(applications.size(), 1);
    QCOMPARE(applications.first().id_, QStringLiteral("legacy-job"));
    QCOMPARE(applications.first().companyId_, storedCompanies.first().id_);
    QCOMPARE(applications.first().cvId_, QStringLiteral("legacy-cv"));
    QCOMPARE(applications.first().techStack_, QStringList{QStringLiteral("Qt")});

    QSqlQuery foreignKeyCheck{database.connection()};
    QVERIFY(foreignKeyCheck.exec(QStringLiteral("PRAGMA foreign_key_check")));
    QVERIFY(!foreignKeyCheck.next());
    QSqlQuery version{database.connection()};
    QVERIFY(version.exec(QStringLiteral("PRAGMA user_version")));
    QVERIFY(version.next());
    QCOMPARE(version.value(0).toInt(), 4);
}

void MigrationTest::migratesVersionTwoCompanyIdentity()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    QString setupError;
    QVERIFY2(
        createVersionTwoDatabase(
            storage.paths().databasePath(),
            {QStringLiteral("  Acme  "), QStringLiteral("acme")},
            setupError),
        qPrintable(setupError));

    SqliteDatabase database{storage.paths().databasePath()};
    CompanyRepository companies{database.connection()};
    JobRepository jobs{database.connection()};
    const auto storedCompanies = companies.findAll();
    const auto applications = jobs.findAll();
    QCOMPARE(storedCompanies.size(), 1);
    QCOMPARE(storedCompanies.first().name_, QStringLiteral("Acme"));
    QCOMPARE(applications.size(), 2);
    QCOMPARE(applications.at(0).companyId_, storedCompanies.first().id_);
    QCOMPARE(applications.at(1).companyId_, storedCompanies.first().id_);

    QSqlQuery normalizedName{database.connection()};
    QVERIFY(normalizedName.exec(QStringLiteral("SELECT normalized_name FROM companies")));
    QVERIFY(normalizedName.next());
    QCOMPARE(normalizedName.value(0).toString(), QStringLiteral("acme"));

    bool hasCompanyForeignKey = false;
    QSqlQuery foreignKeys{database.connection()};
    QVERIFY(foreignKeys.exec(QStringLiteral("PRAGMA foreign_key_list(jobs)")));
    while (foreignKeys.next()) {
        if (foreignKeys.value(QStringLiteral("table")).toString() == QStringLiteral("companies")
            && foreignKeys.value(QStringLiteral("from")).toString() == QStringLiteral("company_id")) {
            hasCompanyForeignKey = true;
        }
    }
    QVERIFY(hasCompanyForeignKey);
    QSqlQuery version{database.connection()};
    QVERIFY(version.exec(QStringLiteral("PRAGMA user_version")));
    QVERIFY(version.next());
    QCOMPARE(version.value(0).toInt(), 4);
}

void MigrationTest::migratesVersionThreeArchiveState()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    QString setupError;
    QVERIFY2(
        createVersionThreeDatabase(storage.paths().databasePath(), setupError),
        qPrintable(setupError));

    SqliteDatabase database{storage.paths().databasePath()};
    CvRepository cvs{database.connection()};
    JobRepository jobs{database.connection()};
    const auto documents = cvs.findAll();
    const auto applications = jobs.findAll();

    QCOMPARE(documents.size(), 1);
    QCOMPARE(documents.first().id_, QStringLiteral("v3-cv"));
    QVERIFY(!documents.first().archivedAt_.isValid());
    QVERIFY(documents.first().isFavorite_);
    QCOMPARE(documents.first().linkedApplicationIds_, QStringList{QStringLiteral("v3-job")});
    QCOMPARE(applications.size(), 1);
    QCOMPARE(applications.first().cvId_, QStringLiteral("v3-cv"));

    QSqlQuery version{database.connection()};
    QVERIFY(version.exec(QStringLiteral("PRAGMA user_version")));
    QVERIFY(version.next());
    QCOMPARE(version.value(0).toInt(), 4);
}

void MigrationTest::rejectsBlankLegacyCompanyTransactionally()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    QString setupError;
    QVERIFY2(
        createVersionTwoDatabase(
            storage.paths().databasePath(),
            {QStringLiteral("  \t  ")},
            setupError),
        qPrintable(setupError));

    const auto connectionName = QStringLiteral("jobtracker-invalid-migration-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(storage.paths().databasePath());
    QVERIFY(database.open());
    QSqlQuery pragmas{database};
    QVERIFY(pragmas.exec(QStringLiteral("PRAGMA foreign_keys = ON")));

    QString migrationError;
    try {
        SchemaMigrator::migrate(database);
    } catch (const std::exception& error) {
        migrationError = QString::fromUtf8(error.what());
    }
    QVERIFY(migrationError.contains(QStringLiteral("blank company name")));

    QSqlQuery version{database};
    QVERIFY(version.exec(QStringLiteral("PRAGMA user_version")));
    QVERIFY(version.next());
    QCOMPARE(version.value(0).toInt(), 2);

    QSqlQuery companiesTable{database};
    QVERIFY(companiesTable.exec(QStringLiteral(
        "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'companies'")));
    QVERIFY(companiesTable.next());
    QCOMPARE(companiesTable.value(0).toInt(), 0);

    QSqlQuery jobs{database};
    QVERIFY(jobs.exec(QStringLiteral("SELECT company_name FROM jobs")));
    QVERIFY(jobs.next());
    QCOMPARE(jobs.value(0).toString(), QStringLiteral("  \t  "));

    database.close();
    database = {};
    QSqlDatabase::removeDatabase(connectionName);
}

QTEST_GUILESS_MAIN(MigrationTest)

#include "MigrationTest.moc"

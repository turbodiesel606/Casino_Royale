#include "SchemaMigrator.hpp"

#include "utils/Utils.hpp"

#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVector>

#include <stdexcept>
#include <utility>

namespace {

constexpr int latestSchemaVersion = 3;

struct LegacyJobCompany
{
    QString jobId_;
    QString displayName_;
    QString normalizedName_;
    QString createdAt_;
    QString updatedAt_;
};

int schemaVersion(QSqlDatabase& database)
{
    QSqlQuery query{database};
    if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) {
        throw std::runtime_error(query.lastError().text().toStdString());
    }

    return query.value(0).toInt();
}

QString normalizedCompanyName(const QString& name)
{
    return name.trimmed().toCaseFolded();
}

void createCvTable(QSqlDatabase& database, const QString& tableName)
{
    utils::executeQuery(database, QStringLiteral(
        "CREATE TABLE %1 ("
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
        "UNIQUE (sha256, original_file_name))")
        .arg(tableName));
}

void createCompaniesTable(QSqlDatabase& database, const QString& tableName)
{
    utils::executeQuery(database, QStringLiteral(
        "CREATE TABLE %1 ("
        "id TEXT PRIMARY KEY,"
        "display_name TEXT NOT NULL CHECK (length(trim(display_name)) > 0),"
        "normalized_name TEXT NOT NULL UNIQUE CHECK (length(normalized_name) > 0),"
        "created_at TEXT NOT NULL,"
        "updated_at TEXT NOT NULL)")
        .arg(tableName));
}

void createVersionTwoJobsTable(
    QSqlDatabase& database,
    const QString& tableName,
    const QString& cvTableName)
{
    utils::executeQuery(database, QStringLiteral(
        "CREATE TABLE %1 ("
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
        "FOREIGN KEY (cv_id) REFERENCES %2(id))")
        .arg(tableName, cvTableName));
}

void createVersionThreeJobsTable(
    QSqlDatabase& database,
    const QString& tableName,
    const QString& cvTableName,
    const QString& companiesTableName)
{
    utils::executeQuery(database, QStringLiteral(
        "CREATE TABLE %1 ("
        "id TEXT PRIMARY KEY,"
        "company_id TEXT NOT NULL,"
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
        "FOREIGN KEY (company_id) REFERENCES %3(id),"
        "FOREIGN KEY (cv_id) REFERENCES %2(id))")
        .arg(tableName, cvTableName, companiesTableName));
}

void createJobTechnologiesTable(
    QSqlDatabase& database,
    const QString& tableName,
    const QString& jobsTableName)
{
    utils::executeQuery(database, QStringLiteral(
        "CREATE TABLE %1 ("
        "job_id TEXT NOT NULL,"
        "position INTEGER NOT NULL,"
        "technology TEXT NOT NULL,"
        "PRIMARY KEY (job_id, position),"
        "FOREIGN KEY (job_id) REFERENCES %2(id) ON DELETE CASCADE)")
        .arg(tableName, jobsTableName));
}

void createVersionTwoTables(
    QSqlDatabase& database,
    const QString& cvTableName,
    const QString& jobsTableName,
    const QString& technologiesTableName)
{
    createCvTable(database, cvTableName);
    createVersionTwoJobsTable(database, jobsTableName, cvTableName);
    createJobTechnologiesTable(database, technologiesTableName, jobsTableName);
}

void createVersionThreeTables(QSqlDatabase& database)
{
    createCvTable(database, QStringLiteral("cvs"));
    createCompaniesTable(database, QStringLiteral("companies"));
    createVersionThreeJobsTable(
        database,
        QStringLiteral("jobs"),
        QStringLiteral("cvs"),
        QStringLiteral("companies"));
    createJobTechnologiesTable(
        database,
        QStringLiteral("job_technologies"),
        QStringLiteral("jobs"));
}

void initializeVersionThree(QSqlDatabase& database)
{
    createVersionThreeTables(database);
    utils::executeQuery(database, QStringLiteral("PRAGMA user_version = 3"));
}

void verifyForeignKeys(QSqlDatabase& database)
{
    QSqlQuery query{database};
    if (!query.exec(QStringLiteral("PRAGMA foreign_key_check"))) {
        utils::throwQueryError(query);
    }

    if (query.next()) {
        throw std::runtime_error(
            "The JobTracker database contains invalid foreign-key relationships.");
    }
}

void migrateVersionOneToTwo(QSqlDatabase& database)
{
    createVersionTwoTables(
        database,
        QStringLiteral("cvs_v2"),
        QStringLiteral("jobs_v2"),
        QStringLiteral("job_technologies_v2"));

    utils::executeQuery(database, QStringLiteral(
        "INSERT INTO cvs_v2 "
        "SELECT id, original_file_name, stored_file_name, relative_path, sha256, size_bytes, "
        "title, category, language, description, is_favorite, created_at, updated_at "
        "FROM cvs"));
    utils::executeQuery(database, QStringLiteral(
        "INSERT INTO jobs_v2 "
        "SELECT id, company_name, job_title, job_url, work_format, city, salary, status, "
        "applied_date, next_step, cv_id, description, requirements, notes, created_at, updated_at "
        "FROM jobs"));
    utils::executeQuery(database, QStringLiteral(
        "INSERT INTO job_technologies_v2 "
        "SELECT job_id, position, technology FROM job_technologies"));

    utils::executeQuery(database, QStringLiteral("DROP TABLE job_technologies"));
    utils::executeQuery(database, QStringLiteral("DROP TABLE jobs"));
    utils::executeQuery(database, QStringLiteral("DROP TABLE cvs"));

    utils::executeQuery(database, QStringLiteral("ALTER TABLE cvs_v2 RENAME TO cvs"));
    utils::executeQuery(database, QStringLiteral("ALTER TABLE jobs_v2 RENAME TO jobs"));
    utils::executeQuery(database, QStringLiteral(
        "ALTER TABLE job_technologies_v2 RENAME TO job_technologies"));

    utils::executeQuery(database, QStringLiteral("PRAGMA user_version = 2"));
}

QVector<LegacyJobCompany> legacyJobCompanies(QSqlDatabase& database)
{
    QSqlQuery query{database};
    if (!query.exec(QStringLiteral(
            "SELECT id, company_name, created_at, updated_at "
            "FROM jobs ORDER BY created_at, id"))) {
        utils::throwQueryError(query);
    }

    QVector<LegacyJobCompany> companies;
    while (query.next()) {
        LegacyJobCompany company;
        company.jobId_ = query.value(QStringLiteral("id")).toString();
        company.displayName_ = query.value(QStringLiteral("company_name")).toString().trimmed();
        company.normalizedName_ = normalizedCompanyName(company.displayName_);
        company.createdAt_ = query.value(QStringLiteral("created_at")).toString();
        company.updatedAt_ = query.value(QStringLiteral("updated_at")).toString();

        if (company.normalizedName_.isEmpty()) {
            throw std::runtime_error(
                QStringLiteral(
                    "Cannot migrate the JobTracker database to schema version 3: "
                    "job '%1' has a blank company name.")
                    .arg(company.jobId_)
                    .toStdString());
        }

        companies.append(std::move(company));
    }

    return companies;
}

QHash<QString, QString> insertMigratedCompanies(
    QSqlDatabase& database,
    const QVector<LegacyJobCompany>& legacyCompanies)
{
    QHash<QString, QString> companyIds;
    QSqlQuery query{database};
    query.prepare(QStringLiteral(
        "INSERT INTO companies (id, display_name, normalized_name, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?)"));

    for (const auto& legacyCompany : legacyCompanies) {
        if (companyIds.contains(legacyCompany.normalizedName_)) {
            continue;
        }

        const auto companyId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        query.bindValue(0, companyId);
        query.bindValue(1, legacyCompany.displayName_);
        query.bindValue(2, legacyCompany.normalizedName_);
        query.bindValue(3, legacyCompany.createdAt_);
        query.bindValue(4, legacyCompany.updatedAt_);
        if (!query.exec()) {
            utils::throwQueryError(query);
        }

        companyIds.insert(legacyCompany.normalizedName_, companyId);
    }

    return companyIds;
}

void copyVersionTwoJobs(
    QSqlDatabase& database,
    const QHash<QString, QString>& companyIds)
{
    QSqlQuery sourceQuery{database};
    if (!sourceQuery.exec(QStringLiteral(
            "SELECT id, company_name, job_title, job_url, work_format, city, salary, status, "
            "applied_date, next_step, cv_id, description, requirements, notes, created_at, updated_at "
            "FROM jobs"))) {
        utils::throwQueryError(sourceQuery);
    }

    QSqlQuery insertQuery{database};
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO jobs_v3 ("
        "id, company_id, job_title, job_url, work_format, city, salary, status, applied_date, "
        "next_step, cv_id, description, requirements, notes, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

    while (sourceQuery.next()) {
        const auto normalizedName = normalizedCompanyName(
            sourceQuery.value(QStringLiteral("company_name")).toString());
        const auto companyId = companyIds.value(normalizedName);
        if (companyId.isEmpty()) {
            throw std::runtime_error(
                "Cannot migrate the JobTracker database to schema version 3: "
                "a company identity could not be resolved.");
        }

        insertQuery.bindValue(0, sourceQuery.value(QStringLiteral("id")));
        insertQuery.bindValue(1, companyId);
        insertQuery.bindValue(2, sourceQuery.value(QStringLiteral("job_title")));
        insertQuery.bindValue(3, sourceQuery.value(QStringLiteral("job_url")));
        insertQuery.bindValue(4, sourceQuery.value(QStringLiteral("work_format")));
        insertQuery.bindValue(5, sourceQuery.value(QStringLiteral("city")));
        insertQuery.bindValue(6, sourceQuery.value(QStringLiteral("salary")));
        insertQuery.bindValue(7, sourceQuery.value(QStringLiteral("status")));
        insertQuery.bindValue(8, sourceQuery.value(QStringLiteral("applied_date")));
        insertQuery.bindValue(9, sourceQuery.value(QStringLiteral("next_step")));
        insertQuery.bindValue(10, sourceQuery.value(QStringLiteral("cv_id")));
        insertQuery.bindValue(11, sourceQuery.value(QStringLiteral("description")));
        insertQuery.bindValue(12, sourceQuery.value(QStringLiteral("requirements")));
        insertQuery.bindValue(13, sourceQuery.value(QStringLiteral("notes")));
        insertQuery.bindValue(14, sourceQuery.value(QStringLiteral("created_at")));
        insertQuery.bindValue(15, sourceQuery.value(QStringLiteral("updated_at")));
        if (!insertQuery.exec()) {
            utils::throwQueryError(insertQuery);
        }
    }
}

void migrateVersionTwoToThree(QSqlDatabase& database)
{
    const auto legacyCompanies = legacyJobCompanies(database);

    createCompaniesTable(database, QStringLiteral("companies"));
    createVersionThreeJobsTable(
        database,
        QStringLiteral("jobs_v3"),
        QStringLiteral("cvs"),
        QStringLiteral("companies"));
    createJobTechnologiesTable(
        database,
        QStringLiteral("job_technologies_v3"),
        QStringLiteral("jobs_v3"));

    const auto companyIds = insertMigratedCompanies(database, legacyCompanies);
    copyVersionTwoJobs(database, companyIds);
    utils::executeQuery(database, QStringLiteral(
        "INSERT INTO job_technologies_v3 "
        "SELECT job_id, position, technology FROM job_technologies"));

    utils::executeQuery(database, QStringLiteral("DROP TABLE job_technologies"));
    utils::executeQuery(database, QStringLiteral("DROP TABLE jobs"));
    utils::executeQuery(database, QStringLiteral("ALTER TABLE jobs_v3 RENAME TO jobs"));
    utils::executeQuery(database, QStringLiteral(
        "ALTER TABLE job_technologies_v3 RENAME TO job_technologies"));

    utils::executeQuery(database, QStringLiteral("PRAGMA user_version = 3"));
}

}

void SchemaMigrator::migrate(QSqlDatabase& database)
{
    const int version = schemaVersion(database);
    if (version == latestSchemaVersion) {
        return;
    }

    if (version > latestSchemaVersion) {
        throw std::runtime_error(
            "The JobTracker database schema is newer than this application supports.");
    }

    if (!database.transaction()) {
        throw std::runtime_error(database.lastError().text().toStdString());
    }

    try {
        if (version == 0) {
            initializeVersionThree(database);
        } else {
            if (version == 1) {
                migrateVersionOneToTwo(database);
            }
            if (version <= 2) {
                migrateVersionTwoToThree(database);
            }
        }

        verifyForeignKeys(database);
        if (!database.commit()) {
            throw std::runtime_error(database.lastError().text().toStdString());
        }
    } catch (...) {
        database.rollback();
        throw;
    }
}

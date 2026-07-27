#include "SchemaMigrationSteps.hpp"

#include "SqlQuery.hpp"

#include <QHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QUuid>
#include <QVector>

#include <stdexcept>
#include <utility>

namespace {

struct LegacyJobCompany
{
    QString jobId_;
    QString displayName_;
    QString normalizedName_;
    QString createdAt_;
    QString updatedAt_;
};

QString normalizedCompanyName(const QString& name)
{
    return name.trimmed().toCaseFolded();
}

QVector<LegacyJobCompany> legacyJobCompanies(QSqlDatabase& database)
{
    QSqlQuery query{database};
    if (!query.exec(QStringLiteral(
            "SELECT id, company_name, created_at, updated_at "
            "FROM jobs ORDER BY created_at, id"))) {
        storage::sql::throwQueryError(
            query,
            QStringLiteral("load legacy companies for schema v3 migration"));
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
            storage::sql::throwQueryError(
                query,
                QStringLiteral("insert a company during schema v3 migration"));
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
        storage::sql::throwQueryError(
            sourceQuery,
            QStringLiteral("load jobs for schema v3 migration"));
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
            storage::sql::throwQueryError(
                insertQuery,
                QStringLiteral("copy a job into schema v3"));
        }
    }
}

}

namespace storage::migrations {

void migrateVersionTwoToThree(QSqlDatabase& database)
{
    const auto legacyCompanies = legacyJobCompanies(database);

    sql::execute(database, QStringLiteral(
        "CREATE TABLE companies ("
        "id TEXT PRIMARY KEY,"
        "display_name TEXT NOT NULL CHECK (length(trim(display_name)) > 0),"
        "normalized_name TEXT NOT NULL UNIQUE CHECK (length(normalized_name) > 0),"
        "created_at TEXT NOT NULL,"
        "updated_at TEXT NOT NULL)"),
        QStringLiteral("create companies table for schema v3 migration"));

    sql::execute(database, QStringLiteral(
        "CREATE TABLE jobs_v3 ("
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
        "FOREIGN KEY (company_id) REFERENCES companies(id),"
        "FOREIGN KEY (cv_id) REFERENCES cvs(id))"),
        QStringLiteral("create jobs replacement table for schema v3 migration"));

    sql::execute(database, QStringLiteral(
        "CREATE TABLE job_technologies_v3 ("
        "job_id TEXT NOT NULL,"
        "position INTEGER NOT NULL,"
        "technology TEXT NOT NULL,"
        "PRIMARY KEY (job_id, position),"
        "FOREIGN KEY (job_id) REFERENCES jobs_v3(id) ON DELETE CASCADE)"),
        QStringLiteral("create job technologies replacement table for schema v3 migration"));

    const auto companyIds = insertMigratedCompanies(database, legacyCompanies);
    copyVersionTwoJobs(database, companyIds);
    sql::execute(database, QStringLiteral(
        "INSERT INTO job_technologies_v3 "
        "SELECT job_id, position, technology FROM job_technologies"),
        QStringLiteral("copy job technologies into schema v3"));

    sql::execute(
        database,
        QStringLiteral("DROP TABLE job_technologies"),
        QStringLiteral("drop schema v2 job technologies table"));
    sql::execute(
        database,
        QStringLiteral("DROP TABLE jobs"),
        QStringLiteral("drop schema v2 jobs table"));
    sql::execute(
        database,
        QStringLiteral("ALTER TABLE jobs_v3 RENAME TO jobs"),
        QStringLiteral("activate schema v3 jobs table"));
    sql::execute(
        database,
        QStringLiteral("ALTER TABLE job_technologies_v3 RENAME TO job_technologies"),
        QStringLiteral("activate schema v3 job technologies table"));
    sql::execute(
        database,
        QStringLiteral("PRAGMA user_version = 3"),
        QStringLiteral("record schema migration from version 2 to 3"));
}

}

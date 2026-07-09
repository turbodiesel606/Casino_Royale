#include "SchemaMigrator.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <stdexcept>

namespace {

void execute(QSqlDatabase& database, const QString& statement)
{
    QSqlQuery query(database);
    if (!query.exec(statement)) {
        throw std::runtime_error(query.lastError().text().toStdString());
    }
}

int schemaVersion(QSqlDatabase& database)
{
    QSqlQuery query(database);
    if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) {
        throw std::runtime_error(query.lastError().text().toStdString());
    }
    return query.value(0).toInt();
}

void migrateToVersionOne(QSqlDatabase& database)
{
    execute(database, QStringLiteral(
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
        "updated_at TEXT NOT NULL)"));
    execute(database, QStringLiteral(
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
        "FOREIGN KEY (cv_id) REFERENCES cvs(id))"));
    execute(database, QStringLiteral(
        "CREATE TABLE job_technologies ("
        "job_id TEXT NOT NULL,"
        "position INTEGER NOT NULL,"
        "technology TEXT NOT NULL,"
        "PRIMARY KEY (job_id, position),"
        "FOREIGN KEY (job_id) REFERENCES jobs(id) ON DELETE CASCADE)"));
    execute(database, QStringLiteral("PRAGMA user_version = 1"));
}

}

void SchemaMigrator::migrate(QSqlDatabase& database)
{
    const auto version = schemaVersion(database);
    if (version > 1) {
        throw std::runtime_error("The JobTracker database schema is newer than this application supports.");
    }
    if (version == 1) {
        return;
    }

    if (!database.transaction()) {
        throw std::runtime_error(database.lastError().text().toStdString());
    }

    try {
        migrateToVersionOne(database);
        if (!database.commit()) {
            throw std::runtime_error(database.lastError().text().toStdString());
        }
    } catch (...) {
        database.rollback();
        throw;
    }
}

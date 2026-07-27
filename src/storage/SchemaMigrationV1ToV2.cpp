#include "SchemaMigrationSteps.hpp"

#include "SqlQuery.hpp"

#include <QSqlDatabase>
#include <QString>

namespace storage::migrations {

void migrateVersionOneToTwo(QSqlDatabase& database)
{
    sql::execute(database, QStringLiteral(
        "CREATE TABLE cvs_v2 ("
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
        QStringLiteral("create schema v2 CV replacement table"));

    sql::execute(database, QStringLiteral(
        "CREATE TABLE jobs_v2 ("
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
        "FOREIGN KEY (cv_id) REFERENCES cvs_v2(id))"),
        QStringLiteral("create schema v2 jobs replacement table"));

    sql::execute(database, QStringLiteral(
        "CREATE TABLE job_technologies_v2 ("
        "job_id TEXT NOT NULL,"
        "position INTEGER NOT NULL,"
        "technology TEXT NOT NULL,"
        "PRIMARY KEY (job_id, position),"
        "FOREIGN KEY (job_id) REFERENCES jobs_v2(id) ON DELETE CASCADE)"),
        QStringLiteral("create schema v2 job technologies replacement table"));

    sql::execute(database, QStringLiteral(
        "INSERT INTO cvs_v2 "
        "SELECT id, original_file_name, stored_file_name, relative_path, sha256, size_bytes, "
        "title, category, language, description, is_favorite, created_at, updated_at "
        "FROM cvs"),
        QStringLiteral("copy CV rows into schema v2"));
    sql::execute(database, QStringLiteral(
        "INSERT INTO jobs_v2 "
        "SELECT id, company_name, job_title, job_url, work_format, city, salary, status, "
        "applied_date, next_step, cv_id, description, requirements, notes, created_at, updated_at "
        "FROM jobs"),
        QStringLiteral("copy job rows into schema v2"));
    sql::execute(database, QStringLiteral(
        "INSERT INTO job_technologies_v2 "
        "SELECT job_id, position, technology FROM job_technologies"),
        QStringLiteral("copy job technologies into schema v2"));

    sql::execute(
        database,
        QStringLiteral("DROP TABLE job_technologies"),
        QStringLiteral("drop schema v1 job technologies table"));
    sql::execute(
        database,
        QStringLiteral("DROP TABLE jobs"),
        QStringLiteral("drop schema v1 jobs table"));
    sql::execute(
        database,
        QStringLiteral("DROP TABLE cvs"),
        QStringLiteral("drop schema v1 CV table"));

    sql::execute(
        database,
        QStringLiteral("ALTER TABLE cvs_v2 RENAME TO cvs"),
        QStringLiteral("activate schema v2 CV table"));
    sql::execute(
        database,
        QStringLiteral("ALTER TABLE jobs_v2 RENAME TO jobs"),
        QStringLiteral("activate schema v2 jobs table"));
    sql::execute(
        database,
        QStringLiteral("ALTER TABLE job_technologies_v2 RENAME TO job_technologies"),
        QStringLiteral("activate schema v2 job technologies table"));
    sql::execute(
        database,
        QStringLiteral("PRAGMA user_version = 2"),
        QStringLiteral("record schema migration from version 1 to 2"));
}

}

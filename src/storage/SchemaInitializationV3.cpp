#include "SchemaMigrationSteps.hpp"

#include "SqlQuery.hpp"

#include <QSqlDatabase>
#include <QString>

namespace storage::migrations {

	void initializeVersionThree(QSqlDatabase& database)
	{
		sql::execute(database, QStringLiteral(
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
			QStringLiteral("create schema v3 CV table"));

		sql::execute(database, QStringLiteral(
			"CREATE TABLE companies ("
			"id TEXT PRIMARY KEY,"
			"display_name TEXT NOT NULL CHECK (length(trim(display_name)) > 0),"
			"normalized_name TEXT NOT NULL UNIQUE CHECK (length(normalized_name) > 0),"
			"created_at TEXT NOT NULL,"
			"updated_at TEXT NOT NULL)"),
			QStringLiteral("create schema v3 companies table"));

		sql::execute(database, QStringLiteral(
			"CREATE TABLE jobs ("
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
			QStringLiteral("create schema v3 jobs table"));

		sql::execute(database, QStringLiteral(
			"CREATE TABLE job_technologies ("
			"job_id TEXT NOT NULL,"
			"position INTEGER NOT NULL,"
			"technology TEXT NOT NULL,"
			"PRIMARY KEY (job_id, position),"
			"FOREIGN KEY (job_id) REFERENCES jobs(id) ON DELETE CASCADE)"),
			QStringLiteral("create schema v3 job technologies table"));

		sql::execute(
			database,
			QStringLiteral("PRAGMA user_version = 3"),
			QStringLiteral("record schema version 3 initialization"));
	}

}

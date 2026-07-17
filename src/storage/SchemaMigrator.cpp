#include "SchemaMigrator.hpp"
#include "utils/Utils.hpp"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <stdexcept>

/*********************************************************************************************************************
* Code uses PRAGMA user_version as the indicator:																	 *
*	version == 0 ? the database is treated as not initialized, so the tables are created and create version 1 schema.*
*	version == 1 ? schema version 1 is already installed, so nothing is created.									 *
*	version > 1 ? the database is newer than the application supports, so the application throws an error.			 *
*																													 *
* But there is an important detail: the code checks the version number, not whether the tables physically exist.	 *
*********************************************************************************************************************/

namespace {

	int schemaVersion(QSqlDatabase& database)
	{
		QSqlQuery query(database); // create a Query object for given DB
		if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) 
			// check for execute SQL command and confirm that query actually returned at least one row (next)
			throw std::runtime_error(query.lastError().text().toStdString());
		
		return query.value(0).toInt(); // return schema version
	}

	void migrateToVersionOne(QSqlDatabase& database)
	{ // build the first supported schema, then stamp the DB as version 1
		utils::executeQuery(database, QStringLiteral(
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
		utils::executeQuery(database, QStringLiteral(
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
		utils::executeQuery(database, QStringLiteral(
			"CREATE TABLE job_technologies ("
			"job_id TEXT NOT NULL,"
			"position INTEGER NOT NULL,"
			"technology TEXT NOT NULL,"
			"PRIMARY KEY (job_id, position),"
			"FOREIGN KEY (job_id) REFERENCES jobs(id) ON DELETE CASCADE)"));
		utils::executeQuery(database, QStringLiteral("PRAGMA user_version = 1"));
	}

}

void SchemaMigrator::migrate(QSqlDatabase& database)
{
	const int version = schemaVersion(database);
	if (version == 1) // DB is already up to date
		return;

	if (version > 1)
		/*************************************************************************************************
		* Checks whether the database version is higher than the app knows how to handle DB.			 *
		* Example: if the app only supports schema version 1, but the database is already version 2 or 3 *
		* (someone, build or tests changed it), this app may be too old to work with new schema.		 *
		*************************************************************************************************/
		throw std::runtime_error("The JobTracker database schema is newer than this application supports.");

	if (!database.transaction()) { // A transaction groups multiple database changes into one safe unit.
		throw std::runtime_error(database.lastError().text().toStdString());
	} 

	try {
		migrateToVersionOne(database);
		if (!database.commit()) { // commit the transaction 
			throw std::runtime_error(database.lastError().text().toStdString());
		}
	}
	catch (...) {
		database.rollback();// Cancel the transaction
		throw;
	}
}

#include "SqliteDatabase.hpp"

#include "SchemaMigrator.hpp"
#include "SqlQuery.hpp"
#include "logging/ApplicationLog.hpp"

#include <QUuid>

SqliteDatabase::SqliteDatabase(const QString& databasePath, LogFact operation, qint64 affectedCount)
// Create a unique name for this Qt database connection.
	: connectionName_{ QStringLiteral("jobtracker-%1").arg(
		  QUuid::createUuid().toString(QUuid::WithoutBraces)) }
	// Create and register the SQLite connection under that unique name.
	, database_{ QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_) }
{
	auto stage = LogFact::DbOpen;
	try {
		database_.setDatabaseName(databasePath);
		if (!database_.open()) {
			/*
			1. open database.sqlite if it already exists;
			2. create a new empty database.sqlite if it does not exist;
			3. return failure if something gone wrong
			*/
			storage::sql::throwDatabaseError(
				database_,
				QStringLiteral("open the JobTracker SQLite database"));
		}

		// Enable foreign-key enforcement for this SQLite connection.
		stage = LogFact::DbForeignKeys;
		storage::sql::execute(
			database_,
			QStringLiteral("PRAGMA foreign_keys = ON"),
			QStringLiteral("enable SQLite foreign-key enforcement"));

		// Wait up to 3 seconds when the SQLite database is temporarily busy.
		stage = LogFact::DbBusyTimeout;
		storage::sql::execute(
			database_,
			QStringLiteral("PRAGMA busy_timeout = 3000"),
			QStringLiteral("configure the SQLite busy timeout"));
		stage = LogFact::SchemaMigration;
		SchemaMigrator::migrate(database_);
	}
	catch (...) {
		if (stage != LogFact::SchemaMigration) {
			const auto cause = database_.isValid() ? storage::sql::currentFailureCause() : LogFact::DbDriverUnavailable;
			const LogValue affected = operation == LogFact::Startup ? LogValue{LogFact::None} : LogValue{affectedCount};
			ApplicationLog::record(LogEventType::DatabaseConnectionSetupFailed, stage, operation, cause, affected);
		}
		closeAndRemoveConnection();
		throw;
	}
}

SqliteDatabase::~SqliteDatabase()
{
	closeAndRemoveConnection();
}

void SqliteDatabase::closeAndRemoveConnection()
{
	database_.close();
	database_ = {};
	QSqlDatabase::removeDatabase(connectionName_);
}

QSqlDatabase& SqliteDatabase::connection()
{
	return database_;
}

const QSqlDatabase& SqliteDatabase::connection() const
{
	return database_;
}

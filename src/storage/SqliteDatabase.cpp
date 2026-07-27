#include "SqliteDatabase.hpp"
#include "SchemaMigrator.hpp"
#include "SqlQuery.hpp"
#include <QUuid>

// Owns the application's SQLite connection, applies startup pragmas, and runs schema migration.

SQLiteDataBase::SQLiteDataBase(const QString& DBPath)
// UUID-based connection name to avoid collisions with other connections
	: connectionName_{ QStringLiteral("jobtracker-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)) }
	, DataBase_{ QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_) } // register connection with Qt
{
	DataBase_.setDatabaseName(DBPath);
	if (!DataBase_.open()) {
		storage::sql::throwDatabaseError(
			DataBase_,
			QStringLiteral("open the JobTracker SQLite database"));
	}

	storage::sql::execute(
		DataBase_,
		QStringLiteral("PRAGMA foreign_keys = ON"),
		QStringLiteral("enable SQLite foreign-key enforcement"));
	storage::sql::execute(
		DataBase_,
		QStringLiteral("PRAGMA busy_timeout = 3000"),
		QStringLiteral("configure the SQLite busy timeout"));
	SchemaMigrator::migrate(DataBase_); // ensure DB structure is present and up to date before the rest of the app starts using it
}

SQLiteDataBase::~SQLiteDataBase()
{
	DataBase_.close(); // close connection
	DataBase_ = {}; // reset QSqlDatabase handle to a default empty value
	QSqlDatabase::removeDatabase(connectionName_); // unregisters the named connection from Qt�s global SQL connection registry
}

QSqlDatabase& SQLiteDataBase::connection()
{
	return DataBase_;
}

const QSqlDatabase& SQLiteDataBase::connection() const
{
	return DataBase_;
}

#include "SqliteDatabase.hpp"
#include "SchemaMigrator.hpp"
#include "utils/Utils.hpp"
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include <stdexcept>

// Owns the application's SQLite connection, applies startup pragmas, and runs schema migration.

SQLiteDataBase::SQLiteDataBase(const QString& DBPath)
// UUID-based connection name to avoid collisions with other connections
	: connectionName_{ QStringLiteral("jobtracker-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)) }
	, DataBase_{ QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_) } // register connection with Qt
{
	DataBase_.setDatabaseName(DBPath);
	if (!DataBase_.open()) {
		throw std::runtime_error(DataBase_.lastError().text().toStdString());
	}

	utils::executeQuery(DataBase_, QStringLiteral("PRAGMA foreign_keys = ON")); // enables foreign key enforcement for this SQLite connection
	utils::executeQuery(DataBase_, QStringLiteral("PRAGMA busy_timeout = 3000")); // wait up to 3000 ms if the database is temporarily locked
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

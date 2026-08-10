#include "SqliteDatabase.hpp"

#include "SchemaMigrator.hpp"
#include "SqlQuery.hpp"

#include <QUuid>

SqliteDatabase::SqliteDatabase(const QString& databasePath)
    // Create a unique name for this Qt database connection.
    : connectionName_{QStringLiteral("jobtracker-%1").arg(
          QUuid::createUuid().toString(QUuid::WithoutBraces))} 
    // Create and register the SQLite connection under that unique name.
    , database_{QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_)} 
{
    try {
        database_.setDatabaseName(databasePath);
        if (!database_.open()) {
            storage::sql::throwDatabaseError(
                database_,
                QStringLiteral("open the JobTracker SQLite database"));
        }

        // Enable foreign-key enforcement for this SQLite connection.
        storage::sql::execute(
            database_,
            QStringLiteral("PRAGMA foreign_keys = ON"),
            QStringLiteral("enable SQLite foreign-key enforcement"));

        // Wait up to 3 seconds when the SQLite database is temporarily busy.
        storage::sql::execute(
            database_,
            QStringLiteral("PRAGMA busy_timeout = 3000"),
            QStringLiteral("configure the SQLite busy timeout"));
        SchemaMigrator::migrate(database_);
    } catch (...) {
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

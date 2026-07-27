#include "SqliteDatabase.hpp"

#include "SchemaMigrator.hpp"
#include "SqlQuery.hpp"

#include <QUuid>

SqliteDatabase::SqliteDatabase(const QString& databasePath)
    : connectionName_{QStringLiteral("jobtracker-%1").arg(
          QUuid::createUuid().toString(QUuid::WithoutBraces))}
    , database_{QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_)}
{
    database_.setDatabaseName(databasePath);
    if (!database_.open()) {
        storage::sql::throwDatabaseError(
            database_,
            QStringLiteral("open the JobTracker SQLite database"));
    }

    storage::sql::execute(
        database_,
        QStringLiteral("PRAGMA foreign_keys = ON"),
        QStringLiteral("enable SQLite foreign-key enforcement"));
    storage::sql::execute(
        database_,
        QStringLiteral("PRAGMA busy_timeout = 3000"),
        QStringLiteral("configure the SQLite busy timeout"));
    SchemaMigrator::migrate(database_);
}

SqliteDatabase::~SqliteDatabase()
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

#include "SqliteDatabase.h"

#include "SchemaMigrator.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include <stdexcept>

namespace {

void executePragma(QSqlDatabase& database, const QString& statement)
{
    QSqlQuery query(database);
    if (!query.exec(statement)) {
        throw std::runtime_error(query.lastError().text().toStdString());
    }
}

}

SqliteDatabase::SqliteDatabase(const QString& databasePath)
    : connectionName_(QStringLiteral("jobtracker-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
    , database_(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_))
{
    database_.setDatabaseName(databasePath);
    if (!database_.open()) {
        throw std::runtime_error(database_.lastError().text().toStdString());
    }

    executePragma(database_, QStringLiteral("PRAGMA foreign_keys = ON"));
    executePragma(database_, QStringLiteral("PRAGMA busy_timeout = 3000"));
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

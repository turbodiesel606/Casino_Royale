#include "SchemaMigrator.hpp"

#include "SchemaMigrationSteps.hpp"
#include "SqlQuery.hpp"
#include "SqlTransaction.hpp"

#include <QSqlDatabase>
#include <QSqlQuery>

#include <stdexcept>

namespace {

constexpr int latestSchemaVersion = 3;

int schemaVersion(QSqlDatabase& database)
{
    QSqlQuery query{database};
    if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) {
        storage::sql::throwQueryError(
            query,
            QStringLiteral("read the SQLite schema version"));
    }
    return query.value(0).toInt();
}

void verifyForeignKeys(QSqlDatabase& database)
{
    QSqlQuery query{database};
    if (!query.exec(QStringLiteral("PRAGMA foreign_key_check"))) {
        storage::sql::throwQueryError(
            query,
            QStringLiteral("verify schema migration foreign keys"));
    }
    if (query.next()) {
        throw std::runtime_error(
            "The JobTracker database contains invalid foreign-key relationships.");
    }
}

}

void SchemaMigrator::migrate(QSqlDatabase& database)
{
    int version = schemaVersion(database);
    if (version == latestSchemaVersion) {
        return;
    }
    if (version > latestSchemaVersion) {
        throw std::runtime_error(
            "The JobTracker database schema is newer than this application supports.");
    }

    SqlTransaction transaction{database, QStringLiteral("schema migration")};
    if (version == 0) {
        storage::migrations::initializeVersionThree(database);
        version = latestSchemaVersion;
    }

    while (version < latestSchemaVersion) {
        switch (version) {
        case 1:
            storage::migrations::migrateVersionOneToTwo(database);
            version = 2;
            break;
        case 2:
            storage::migrations::migrateVersionTwoToThree(database);
            version = 3;
            break;
        default:
            throw std::runtime_error(
                "The JobTracker database schema version is unsupported.");
        }
    }

    verifyForeignKeys(database);
    transaction.commit();
}

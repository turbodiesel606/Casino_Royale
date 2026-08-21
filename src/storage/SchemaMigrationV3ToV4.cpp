#include "SchemaMigrationSteps.hpp"

#include "SqlQuery.hpp"

#include <QSqlDatabase>
#include <QString>

namespace storage::migrations {

void migrateVersionThreeToFour(QSqlDatabase& database)
{
    sql::execute(
        database,
        QStringLiteral("ALTER TABLE cvs ADD COLUMN archived_at TEXT DEFAULT NULL"),
        QStringLiteral("add CV archive timestamp"));
    sql::execute(
        database,
        QStringLiteral("PRAGMA user_version = 4"),
        QStringLiteral("record schema version 4 migration"));
}

}

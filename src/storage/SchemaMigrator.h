#pragma once

class QSqlDatabase;

class SchemaMigrator final
{
public:
    static void migrate(QSqlDatabase& database);
};

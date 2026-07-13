#ifndef JOBTRACKER_SRC_STORAGE_SCHEMAMIGRATOR_HPP
#define JOBTRACKER_SRC_STORAGE_SCHEMAMIGRATOR_HPP

class QSqlDatabase;

class SchemaMigrator final
{
public:
    static void migrate(QSqlDatabase& database);
};

#endif // JOBTRACKER_SRC_STORAGE_SCHEMAMIGRATOR_HPP

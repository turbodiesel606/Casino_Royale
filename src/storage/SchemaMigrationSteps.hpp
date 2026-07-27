#ifndef JOBTRACKER_SRC_STORAGE_SCHEMAMIGRATIONSTEPS_HPP
#define JOBTRACKER_SRC_STORAGE_SCHEMAMIGRATIONSTEPS_HPP

class QSqlDatabase;

namespace storage::migrations {

void initializeVersionThree(QSqlDatabase& database);
void migrateVersionOneToTwo(QSqlDatabase& database);
void migrateVersionTwoToThree(QSqlDatabase& database);

}

#endif // JOBTRACKER_SRC_STORAGE_SCHEMAMIGRATIONSTEPS_HPP

#ifndef JOBTRACKER_SRC_STORAGE_SCHEMAMIGRATIONSTEPS_HPP
#define JOBTRACKER_SRC_STORAGE_SCHEMAMIGRATIONSTEPS_HPP

class QSqlDatabase;

namespace storage::migrations {

void initializeVersionFour(QSqlDatabase& database);
void migrateVersionOneToTwo(QSqlDatabase& database);
void migrateVersionTwoToThree(QSqlDatabase& database);
void migrateVersionThreeToFour(QSqlDatabase& database);

}

#endif // JOBTRACKER_SRC_STORAGE_SCHEMAMIGRATIONSTEPS_HPP

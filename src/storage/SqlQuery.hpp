#ifndef JOBTRACKER_SRC_STORAGE_SQLQUERY_HPP
#define JOBTRACKER_SRC_STORAGE_SQLQUERY_HPP

class QSqlDatabase;
class QSqlQuery;
class QString;

namespace storage::sql {

[[noreturn]] void throwDatabaseError(
    const QSqlDatabase& database,
    const QString& operationContext);

[[noreturn]] void throwQueryError(
    const QSqlQuery& query,
    const QString& operationContext);

void execute(
    QSqlDatabase& database,
    const QString& statement,
    const QString& operationContext);

}

#endif // JOBTRACKER_SRC_STORAGE_SQLQUERY_HPP

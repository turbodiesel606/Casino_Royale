#ifndef JOBTRACKER_SRC_STORAGE_SQLQUERY_HPP
#define JOBTRACKER_SRC_STORAGE_SQLQUERY_HPP

#include <QDateTime>
#include <QString>

class QSqlDatabase;
class QSqlQuery;

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

void execute(
    QSqlQuery& query,
    const QString& operationContext);

QString nonNullText(const QString& value);

QDateTime readIsoDateTime(
    const QSqlQuery& query,
    const QString& columnName);

}

#endif // JOBTRACKER_SRC_STORAGE_SQLQUERY_HPP

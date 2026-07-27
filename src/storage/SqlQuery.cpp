#include "SqlQuery.hpp"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

#include <stdexcept>

namespace {

[[noreturn]] void throwSqlError(const QString& operationContext, const QString& errorText)
{
    throw std::runtime_error(
        QStringLiteral("Failed to %1: %2")
            .arg(operationContext, errorText)
            .toStdString());
}

}

namespace storage::sql {

void throwDatabaseError(const QSqlDatabase& database, const QString& operationContext)
{
    throwSqlError(operationContext, database.lastError().text());
}

void throwQueryError(const QSqlQuery& query, const QString& operationContext)
{
    throwSqlError(operationContext, query.lastError().text());
}

void execute(
    QSqlDatabase& database,
    const QString& statement,
    const QString& operationContext)
{
    QSqlQuery query{database};
    if (!query.exec(statement)) {
        throwQueryError(query, operationContext);
    }
}

}

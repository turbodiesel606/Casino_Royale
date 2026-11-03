#ifndef JOBTRACKER_SRC_STORAGE_SQLQUERY_HPP
#define JOBTRACKER_SRC_STORAGE_SQLQUERY_HPP

#include "logging/LogEvent.hpp"

#include <QDateTime>
#include <QString>
#include <stdexcept>

class QSqlDatabase;
class QSqlError;
class QSqlQuery;

namespace storage::sql {

// Codex Comment: Preserve the business-facing error while retaining a bounded diagnostic cause.
class Failure final : public std::runtime_error
{
public:
    Failure(const std::string& message, LogFact cause);
    LogFact cause() const noexcept;
private:
    LogFact cause_;
};

LogFact failureCause(const QSqlError& error) noexcept;
LogFact currentFailureCause() noexcept;
void recordDatasetReadError(const QSqlQuery& query, LogFact dataset) noexcept;

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

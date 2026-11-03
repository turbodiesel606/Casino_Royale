#ifndef JOBTRACKER_SRC_STORAGE_SQLTRANSACTION_HPP
#define JOBTRACKER_SRC_STORAGE_SQLTRANSACTION_HPP

#include "logging/LogEvent.hpp"

#include <QString>

class QSqlDatabase;

// Owns one explicit SQL transaction and rolls it back unless commit succeeds.
class SqlTransaction final
{
public:
    SqlTransaction(QSqlDatabase& database, QString operationContext,
        LogFact operation = LogFact::Unknown, LogFact* rollback = nullptr, quint64 operationId = 0);
    ~SqlTransaction();

    SqlTransaction(const SqlTransaction&) = delete;
    SqlTransaction& operator=(const SqlTransaction&) = delete;

    void commit();

private:
    QSqlDatabase& database_;
    QString operationContext_;
    LogFact operation_;
    LogFact* rollback_;
    quint64 operationId_;
    bool active_ = false;
};

#endif // JOBTRACKER_SRC_STORAGE_SQLTRANSACTION_HPP

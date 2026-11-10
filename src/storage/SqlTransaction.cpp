#include "SqlTransaction.hpp"

#include "SqlQuery.hpp"

#include "logging/ApplicationLog.hpp"
#include <QSqlDatabase>
#include <QSqlError>

#include <utility>

SqlTransaction::SqlTransaction(QSqlDatabase& database, QString operationContext,
    LogFact operation, LogFact* rollback, quint64 operationId)
    : database_(database)
    , operationContext_(std::move(operationContext))
    , operation_{operation}
    , rollback_{rollback}
    , operationId_{operationId}
{
    if (rollback_) *rollback_ = LogFact::NotAttempted;
    if (!database_.transaction()) {
        storage::sql::throwDatabaseError(database_,
            QStringLiteral("start %1").arg(operationContext_));
    }
    active_ = true;
}

SqlTransaction::~SqlTransaction()
{
    if (!active_) return;
    const bool rolledBack = database_.rollback();
    if (rollback_) *rollback_ = rolledBack ? LogFact::Succeeded : LogFact::Failed;
    if (!rolledBack) {
        ApplicationLog::record(operationId_, LogEventType::DatabaseRollbackFailed,
            operation_, storage::sql::failureCause(database_.lastError()));
    }
}

void SqlTransaction::commit()
{
    if (!active_) 
        return;
    
    if (!database_.commit()) {
        storage::sql::throwDatabaseError(
            database_,
            QStringLiteral("commit %1").arg(operationContext_));
    }
    active_ = false;
}

#include "SqlTransaction.hpp"

#include "SqlQuery.hpp"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>

#include <utility>

SqlTransaction::SqlTransaction(QSqlDatabase& database, QString operationContext)
    : database_(database)
    , operationContext_(std::move(operationContext))
{
    if (!database_.transaction()) {
        storage::sql::throwDatabaseError(
            database_,
            QStringLiteral("start %1").arg(operationContext_));
    }
    active_ = true;
}

SqlTransaction::~SqlTransaction()
{
    if (active_ && !database_.rollback()) {
        qWarning().noquote()
            << QStringLiteral("Failed to roll back %1: %2")
                   .arg(operationContext_, database_.lastError().text());
    }
}

void SqlTransaction::commit()
{
    if (!active_) {
        return;
    }
    if (!database_.commit()) {
        storage::sql::throwDatabaseError(
            database_,
            QStringLiteral("commit %1").arg(operationContext_));
    }
    active_ = false;
}

#ifndef JOBTRACKER_SRC_STORAGE_SQLTRANSACTION_HPP
#define JOBTRACKER_SRC_STORAGE_SQLTRANSACTION_HPP

#include <QString>

class QSqlDatabase;

// Owns one explicit SQL transaction and rolls it back unless commit succeeds.
class SqlTransaction final
{
public:
    SqlTransaction(QSqlDatabase& database, QString operationContext);
    ~SqlTransaction();

    SqlTransaction(const SqlTransaction&) = delete;
    SqlTransaction& operator=(const SqlTransaction&) = delete;

    void commit();

private:
    QSqlDatabase& database_;
    QString operationContext_;
    bool active_ = false;
};

#endif // JOBTRACKER_SRC_STORAGE_SQLTRANSACTION_HPP

#ifndef JOBTRACKER_SRC_STORAGE_SQLITEDATABASE_HPP
#define JOBTRACKER_SRC_STORAGE_SQLITEDATABASE_HPP

#include <QSqlDatabase>
#include <QString>

class SqliteDatabase final
{
public:
    explicit SqliteDatabase(const QString& databasePath);
    ~SqliteDatabase();

    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    QSqlDatabase& connection();
    const QSqlDatabase& connection() const;

private:
    QString connectionName_;
    QSqlDatabase database_;
};

#endif // JOBTRACKER_SRC_STORAGE_SQLITEDATABASE_HPP

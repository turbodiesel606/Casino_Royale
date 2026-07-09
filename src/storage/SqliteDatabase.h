#pragma once

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

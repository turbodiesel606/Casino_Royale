#ifndef JOBTRACKER_SRC_STORAGE_SQLITEDATABASE_HPP
#define JOBTRACKER_SRC_STORAGE_SQLITEDATABASE_HPP

#include <QSqlDatabase>
#include <QString>

class SQLiteDataBase final
{
public:
    explicit SQLiteDataBase(const QString& databasePath);
    ~SQLiteDataBase();

    SQLiteDataBase(const SQLiteDataBase&) = delete;
    SQLiteDataBase& operator=(const SQLiteDataBase&) = delete;

    QSqlDatabase& connection();
    const QSqlDatabase& connection() const;

private:
    QString connectionName_;
    QSqlDatabase DataBase_;
};

#endif // JOBTRACKER_SRC_STORAGE_SQLITEDATABASE_HPP

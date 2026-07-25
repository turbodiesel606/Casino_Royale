#ifndef JOBTRACKER_SRC_UTILS_UTILS_HPP
#define JOBTRACKER_SRC_UTILS_UTILS_HPP

// Utilites

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

namespace utils {

    [[noreturn]] void throwQueryError(const QSqlQuery& query);

     void executeQuery(QSqlDatabase& database, const QString& statement);

}

#endif // JOBTRACKER_SRC_UTILS_UTILS_HPP

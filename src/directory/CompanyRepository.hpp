#ifndef JOBTRACKER_SRC_DIRECTORY_COMPANYREPOSITORY_HPP
#define JOBTRACKER_SRC_DIRECTORY_COMPANYREPOSITORY_HPP

#include "Company.hpp"

#include <QVector>

class QSqlDatabase;

// Persists durable company identities independently from their QML presentation.
class CompanyRepository final
{
public:
    explicit CompanyRepository(QSqlDatabase& database);

    QVector<Company> findAll() const;
    Company findOrCreateByName(const QString& name) const;

private:
    QSqlDatabase& database_;
};

#endif // JOBTRACKER_SRC_DIRECTORY_COMPANYREPOSITORY_HPP

#ifndef JOBTRACKER_SRC_CVS_CVREPOSITORY_HPP
#define JOBTRACKER_SRC_CVS_CVREPOSITORY_HPP

#include "CvDocument.hpp"

#include <optional>

class QSqlDatabase;

class CvRepository final
{
public:
    explicit CvRepository(QSqlDatabase& database);

    QVector<CvDocument> findAll() const;
    std::optional<CvDocument> findBySha256(const QString& sha256) const;
    void insert(const CvDocument& document) const;

private:
    QSqlDatabase& database_;
};

#endif // JOBTRACKER_SRC_CVS_CVREPOSITORY_HPP

#pragma once

#include "CvDocument.h"

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

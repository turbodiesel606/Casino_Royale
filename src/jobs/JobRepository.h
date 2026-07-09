#pragma once

#include "JobApplication.h"

class QSqlDatabase;

class JobRepository final
{
public:
    explicit JobRepository(QSqlDatabase& database);

    QVector<JobApplication> findAll() const;
    void insert(const JobApplication& application) const;

private:
    QSqlDatabase& database_;
};

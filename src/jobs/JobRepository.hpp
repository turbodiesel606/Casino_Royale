#ifndef JOBTRACKER_SRC_JOBS_JOBREPOSITORY_HPP
#define JOBTRACKER_SRC_JOBS_JOBREPOSITORY_HPP

#include "JobApplication.hpp"

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

#endif // JOBTRACKER_SRC_JOBS_JOBREPOSITORY_HPP

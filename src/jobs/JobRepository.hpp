#ifndef JOBTRACKER_SRC_JOBS_JOBREPOSITORY_HPP
#define JOBTRACKER_SRC_JOBS_JOBREPOSITORY_HPP

#include "JobApplication.hpp"

#include <optional>
#include <QVector>

class QSqlDatabase;
// Provides SQLite persistence for job applications and their ordered technologies.
// Loads each job with its linked company and CV display data, 
// and inserts new job records within a caller-managed transaction.

class JobRepository final
{
public:
    explicit JobRepository(QSqlDatabase& database);

    QVector<JobApplication> findAll() const;
    std::optional<JobApplication> findById(const QString& applicationId) const;
    void insert(const JobApplication& application) const;
    bool update(const JobApplication& application) const;
    bool remove(const QString& applicationId) const;

private:
    QSqlDatabase& database_;
};

#endif // JOBTRACKER_SRC_JOBS_JOBREPOSITORY_HPP

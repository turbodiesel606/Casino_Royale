#include "JobRepository.hpp"

#include "storage/SqlQuery.hpp"

#include <QHash>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <utility>

namespace {

const QString& baseJobSelect()
{
    static const QString statement = QStringLiteral(
        "SELECT jobs.*, companies.display_name AS company_name, cvs.original_file_name "
        "FROM jobs "
        "JOIN companies ON companies.id = jobs.company_id "
        "JOIN cvs ON cvs.id = jobs.cv_id");
    return statement;
}

JobApplication convertToJobApplication(const QSqlQuery& query)
{
    JobApplication application;
    application.id_ = query.value(QStringLiteral("id")).toString();
    application.companyId_ = query.value(QStringLiteral("company_id")).toString();
    application.companyName_ = query.value(QStringLiteral("company_name")).toString();
    application.jobTitle_ = query.value(QStringLiteral("job_title")).toString();
    application.jobUrl_ = QUrl{
        query.value(QStringLiteral("job_url")).toString(),
        QUrl::StrictMode};
    application.workFormat_ = workFormatFromString(
        query.value(QStringLiteral("work_format")).toString());
    application.city_ = query.value(QStringLiteral("city")).toString();
    application.salary_ = query.value(QStringLiteral("salary")).toString();
    application.status_ = jobStatusFromString(
        query.value(QStringLiteral("status")).toString());
    application.appliedDate_ = QDate::fromString(
        query.value(QStringLiteral("applied_date")).toString(),
        Qt::ISODate);
    application.nextStep_ = query.value(QStringLiteral("next_step")).toString();
    application.cvId_ = query.value(QStringLiteral("cv_id")).toString();
    application.cvFileName_ = query.value(QStringLiteral("original_file_name")).toString();
    application.description_ = query.value(QStringLiteral("description")).toString();
    application.requirements_ = query.value(QStringLiteral("requirements")).toString();
    application.notes_ = query.value(QStringLiteral("notes")).toString();
    application.createdAt_ = storage::sql::readIsoDateTime(
        query,
        QStringLiteral("created_at"));
    application.updatedAt_ = storage::sql::readIsoDateTime(
        query,
        QStringLiteral("updated_at"));
    return application;
}

void bindMutableJobFields(QSqlQuery& query, const JobApplication& application)
{
    query.bindValue(QStringLiteral(":company_id"), application.companyId_);
    query.bindValue(QStringLiteral(":job_title"), application.jobTitle_);
    query.bindValue(
        QStringLiteral(":job_url"),
        storage::sql::nonNullText(application.jobUrl_.toString()));
    query.bindValue(
        QStringLiteral(":work_format"),
        storage::sql::nonNullText(workFormatToString(application.workFormat_)));
    query.bindValue(
        QStringLiteral(":city"),
        storage::sql::nonNullText(application.city_));
    query.bindValue(
        QStringLiteral(":salary"),
        storage::sql::nonNullText(application.salary_));
    query.bindValue(QStringLiteral(":status"), jobStatusToString(application.status_));
    query.bindValue(
        QStringLiteral(":applied_date"),
        application.appliedDate_.toString(Qt::ISODate));
    query.bindValue(
        QStringLiteral(":next_step"),
        storage::sql::nonNullText(application.nextStep_));
    query.bindValue(QStringLiteral(":cv_id"), application.cvId_);
    query.bindValue(
        QStringLiteral(":description"),
        storage::sql::nonNullText(application.description_));
    query.bindValue(
        QStringLiteral(":requirements"),
        storage::sql::nonNullText(application.requirements_));
    query.bindValue(
        QStringLiteral(":notes"),
        storage::sql::nonNullText(application.notes_));
}

void insertTechnologies(QSqlDatabase& database, const JobApplication& application)
{
    QSqlQuery query{database};
    query.prepare(QStringLiteral(
        "INSERT INTO job_technologies (job_id, position, technology) "
        "VALUES (:job_id, :position, :technology)"));

    for (int position = 0; position < application.techStack_.size(); ++position) {
        query.bindValue(QStringLiteral(":job_id"), application.id_);
        query.bindValue(QStringLiteral(":position"), position);
        query.bindValue(
            QStringLiteral(":technology"),
            application.techStack_.at(position));
        storage::sql::execute(
            query,
            QStringLiteral("insert a job application technology"));
    }
}

} // namespace

JobRepository::JobRepository(QSqlDatabase& database)
    : database_{database}
{
}

QVector<JobApplication> JobRepository::findAll() const
{
    QSqlQuery jobsQuery{database_};
    if (!jobsQuery.exec(baseJobSelect() + QStringLiteral(" ORDER BY jobs.created_at DESC"))) {
        storage::sql::throwQueryError(
            jobsQuery,
            QStringLiteral("load job applications"));
    }

    QVector<JobApplication> applications;
    QHash<QString, qsizetype> applicationIndexes;
    while (jobsQuery.next()) {
        auto application = convertToJobApplication(jobsQuery);
        applicationIndexes.insert(application.id_, applications.size());
        applications.append(std::move(application));
    }

    if (applications.isEmpty()) {
        return applications;
    }

    QSqlQuery technologiesQuery{database_};
    if (!technologiesQuery.exec(QStringLiteral(
            "SELECT job_id, technology "
            "FROM job_technologies "
            "ORDER BY job_id, position"))) {
        storage::sql::throwQueryError(
            technologiesQuery,
            QStringLiteral("load job application technologies"));
    }

    while (technologiesQuery.next()) {
        const auto jobId = technologiesQuery.value(QStringLiteral("job_id")).toString();
        const auto indexIt = applicationIndexes.constFind(jobId);
        if (indexIt != applicationIndexes.constEnd()) {
            applications[*indexIt].techStack_.append(
                technologiesQuery.value(QStringLiteral("technology")).toString());
        }
    }
    return applications;
}

std::optional<JobApplication> JobRepository::findById(
    const QString& applicationId) const
{
    QSqlQuery jobQuery{database_};
    jobQuery.prepare(baseJobSelect() + QStringLiteral(" WHERE jobs.id = :id"));
    jobQuery.bindValue(QStringLiteral(":id"), applicationId);
    storage::sql::execute(jobQuery, QStringLiteral("load a job application"));
    if (!jobQuery.next()) {
        return std::nullopt;
    }

    auto application = convertToJobApplication(jobQuery);
    QSqlQuery technologiesQuery{database_};
    technologiesQuery.prepare(QStringLiteral(
        "SELECT technology FROM job_technologies "
        "WHERE job_id = :job_id ORDER BY position"));
    technologiesQuery.bindValue(QStringLiteral(":job_id"), applicationId);
    storage::sql::execute(
        technologiesQuery,
        QStringLiteral("load job application technologies"));
    while (technologiesQuery.next()) {
        application.techStack_.append(technologiesQuery.value(0).toString());
    }
    return application;
}

void JobRepository::insert(const JobApplication& application) const
{
    QSqlQuery query{database_};
    query.prepare(QStringLiteral(
        "INSERT INTO jobs (id, company_id, job_title, job_url, work_format, city, salary,"
        " status, applied_date, next_step, cv_id, description, requirements, notes,"
        " created_at, updated_at) "
        "VALUES (:id, :company_id, :job_title, :job_url, :work_format, :city, :salary,"
        " :status, :applied_date, :next_step, :cv_id, :description, :requirements, :notes,"
        " :created_at, :updated_at)"));
    query.bindValue(QStringLiteral(":id"), application.id_);
    bindMutableJobFields(query, application);
    query.bindValue(
        QStringLiteral(":created_at"),
        application.createdAt_.toUTC().toString(Qt::ISODate));
    query.bindValue(
        QStringLiteral(":updated_at"),
        application.updatedAt_.toUTC().toString(Qt::ISODate));
    storage::sql::execute(query, QStringLiteral("insert a job application"));

    insertTechnologies(database_, application);
}

bool JobRepository::update(const JobApplication& application) const
{
    QSqlQuery query{database_};
    query.prepare(QStringLiteral(
        "UPDATE jobs SET company_id = :company_id, job_title = :job_title,"
        " job_url = :job_url, work_format = :work_format, city = :city, salary = :salary,"
        " status = :status, applied_date = :applied_date, next_step = :next_step,"
        " cv_id = :cv_id, description = :description, requirements = :requirements,"
        " notes = :notes, updated_at = :updated_at WHERE id = :id"));
    bindMutableJobFields(query, application);
    query.bindValue(
        QStringLiteral(":updated_at"),
        application.updatedAt_.toUTC().toString(Qt::ISODateWithMs));
    query.bindValue(QStringLiteral(":id"), application.id_);
    storage::sql::execute(query, QStringLiteral("update a job application"));
    if (query.numRowsAffected() != 1) {
        return false;
    }

    QSqlQuery deleteTechnologies{database_};
    deleteTechnologies.prepare(QStringLiteral(
        "DELETE FROM job_technologies WHERE job_id = :job_id"));
    deleteTechnologies.bindValue(QStringLiteral(":job_id"), application.id_);
    storage::sql::execute(
        deleteTechnologies,
        QStringLiteral("replace job application technologies"));
    insertTechnologies(database_, application);
    return true;
}

bool JobRepository::remove(const QString& applicationId) const
{
    QSqlQuery query{database_};
    query.prepare(QStringLiteral("DELETE FROM jobs WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), applicationId);
    storage::sql::execute(query, QStringLiteral("delete a job application"));
    return query.numRowsAffected() == 1;
}

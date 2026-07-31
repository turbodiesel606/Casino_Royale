#include "JobRepository.hpp"
#include "storage/SqlQuery.hpp"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>

namespace {

	QString sqlText(const QString& value)
	{
		return value.isNull() ? QStringLiteral("") : value;
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
		application.createdAt_ = QDateTime::fromString(
			query.value(QStringLiteral("created_at")).toString(),
			Qt::ISODate);
		application.updatedAt_ = QDateTime::fromString(
			query.value(QStringLiteral("updated_at")).toString(),
			Qt::ISODate);

		return application;
	}
}

JobRepository::JobRepository(QSqlDatabase& database)
	: database_(database)
{
}

// Load all jobs with their linked CV filenames and ordered technology lists.

QVector<JobApplication> JobRepository::findAll() const
{   // Impl reserve logic to conatiners in future
	QSqlQuery jobsQuery(database_);
	if (!jobsQuery.exec(QStringLiteral(
		"SELECT jobs.*, companies.display_name AS company_name, cvs.original_file_name "
		"FROM jobs "
		"JOIN companies ON companies.id = jobs.company_id "
		"JOIN cvs ON cvs.id = jobs.cv_id "
		"ORDER BY jobs.created_at DESC"))) {
		storage::sql::throwQueryError(jobsQuery, QStringLiteral("load job applications"));
	}

	QVector<JobApplication> applications;
	QHash<QString, qsizetype> applicationIndexes;

	while (jobsQuery.next()) {
		JobApplication application = convertToJobApplication(jobsQuery);

		const qsizetype index = applications.size();
		applicationIndexes.insert(application.id_, index);

		applications.append(std::move(application));
	}

	if (applications.isEmpty())
		return applications;

	QSqlQuery technologiesQuery(database_);
	if (!technologiesQuery.exec(QStringLiteral(
		"SELECT job_id, technology "
		"FROM job_technologies "
		"ORDER BY job_id, position"))) {
		storage::sql::throwQueryError(
			technologiesQuery,
			QStringLiteral("load job application technologies"));
	}

	while (technologiesQuery.next()) {
		const QString jobId =
			technologiesQuery.value(QStringLiteral("job_id")).toString();

		const auto indexIt = applicationIndexes.constFind(jobId);

		if (indexIt != applicationIndexes.constEnd()) {
			applications[*indexIt].techStack_.append(
				technologiesQuery.value(QStringLiteral("technology")).toString());
		}
	}

	return applications;
}

void JobRepository::insert(const JobApplication& application) const
{
	QSqlQuery query(database_);
	query.prepare(QStringLiteral(
		"INSERT INTO jobs (id, company_id, job_title, job_url, work_format, city, salary,"
		" status, applied_date, next_step, cv_id, description, requirements, notes, created_at, updated_at)"
		" VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
	query.addBindValue(application.id_);
	query.addBindValue(application.companyId_);
	query.addBindValue(application.jobTitle_);
	query.addBindValue(sqlText(application.jobUrl_.toString()));
	query.addBindValue(sqlText(workFormatToString(application.workFormat_)));
	query.addBindValue(sqlText(application.city_));
	query.addBindValue(sqlText(application.salary_));
	query.addBindValue(jobStatusToString(application.status_));
	query.addBindValue(application.appliedDate_.toString(Qt::ISODate));
	query.addBindValue(sqlText(application.nextStep_));
	query.addBindValue(application.cvId_);
	query.addBindValue(sqlText(application.description_));
	query.addBindValue(sqlText(application.requirements_));
	query.addBindValue(sqlText(application.notes_));
	query.addBindValue(application.createdAt_.toUTC().toString(Qt::ISODate));
	query.addBindValue(application.updatedAt_.toUTC().toString(Qt::ISODate));

	if (!query.exec())
		storage::sql::throwQueryError(query, QStringLiteral("insert a job application"));

	for (int position = 0; position < application.techStack_.size(); ++position) {
		QSqlQuery technologyQuery(database_);
		technologyQuery.prepare(QStringLiteral(
			"INSERT INTO job_technologies (job_id, position, technology) VALUES (?, ?, ?)"));
		technologyQuery.addBindValue(application.id_);
		technologyQuery.addBindValue(position);
		technologyQuery.addBindValue(application.techStack_.at(position));

		if (!technologyQuery.exec())
			storage::sql::throwQueryError(
				technologyQuery,
				QStringLiteral("insert a job application technology"));

	}
}

#include "JobRepository.hpp"
#include"utils/Utils.hpp"
#include <QDate>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <stdexcept>

namespace {

	JobApplication convertToJobApplication(const QSqlQuery& query)
	{
		JobApplication application;
		application.id_ = query.value(QStringLiteral("id")).toString();
		application.companyName_ = query.value(QStringLiteral("company_name")).toString();
		application.companyInitials_ = application.companyName_.left(2).toUpper();
		application.companyAccent_ = QStringLiteral("#146ce0");
		application.jobTitle_ = query.value(QStringLiteral("job_title")).toString();
		application.jobUrl_ = query.value(QStringLiteral("job_url")).toString();
		application.workFormat_ = query.value(QStringLiteral("work_format")).toString();
		application.city_ = query.value(QStringLiteral("city")).toString();
		application.salary_ = query.value(QStringLiteral("salary")).toString();
		application.status_ = query.value(QStringLiteral("status")).toString();
		application.appliedDate_ = query.value(QStringLiteral("applied_date")).toString();
		application.dateLabel_ = QDate::fromString(application.appliedDate_, Qt::ISODate).toString(QStringLiteral("MMM d, yyyy"));
		application.nextStep_ = query.value(QStringLiteral("next_step")).toString();
		application.cvId_ = query.value(QStringLiteral("cv_id")).toString();
		application.cvFileName_ = query.value(QStringLiteral("original_file_name")).toString();
		application.description_ = query.value(QStringLiteral("description")).toString();
		application.requirements_ = query.value(QStringLiteral("requirements")).toString();
		application.notes_ = query.value(QStringLiteral("notes")).toString();

		return application;
	}
}

JobRepository::JobRepository(QSqlDatabase& database)
	: database_(database)
{
}

// Load all jobs with their linked CV filenames and ordered technology lists.

QVector<JobApplication> JobRepository::findAll() const
{
	QSqlQuery jobsQuery(database_);
	if (!jobsQuery.exec(QStringLiteral(
		"SELECT jobs.*, cvs.original_file_name "
		"FROM jobs "
		"JOIN cvs ON cvs.id = jobs.cv_id "
		"ORDER BY jobs.created_at DESC"))) {
		utils::throwQueryError(jobsQuery);
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
		utils::throwQueryError(technologiesQuery);
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
		"INSERT INTO jobs (id, company_name, job_title, job_url, work_format, city, salary,"
		" status, applied_date, next_step, cv_id, description, requirements, notes, created_at, updated_at)"
		" VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
	const auto now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
	query.addBindValue(application.id_);
	query.addBindValue(application.companyName_);
	query.addBindValue(application.jobTitle_);
	query.addBindValue(application.jobUrl_.isNull() ? QString{} : application.jobUrl_);
	query.addBindValue(application.workFormat_.isNull() ? QString{} : application.workFormat_);
	query.addBindValue(application.city_.isNull() ? QString{} : application.city_);
	query.addBindValue(application.salary_.isNull() ? QString{} : application.salary_);
	query.addBindValue(application.status_);
	query.addBindValue(application.appliedDate_);
	query.addBindValue(application.nextStep_.isNull() ? QString{} : application.nextStep_);
	query.addBindValue(application.cvId_);
	query.addBindValue(application.description_.isNull() ? QString{} : application.description_);
	query.addBindValue(application.requirements_.isNull() ? QString{} : application.requirements_);
	query.addBindValue(application.notes_.isNull() ? QString{} : application.notes_);
	query.addBindValue(now);
	query.addBindValue(now);

	if (!query.exec())
		utils::throwQueryError(query);

	for (int position = 0; position < application.techStack_.size(); ++position) {
		QSqlQuery technologyQuery(database_);
		technologyQuery.prepare(QStringLiteral(
			"INSERT INTO job_technologies (job_id, position, technology) VALUES (?, ?, ?)"));
		technologyQuery.addBindValue(application.id_);
		technologyQuery.addBindValue(position);
		technologyQuery.addBindValue(application.techStack_.at(position));

		if (!technologyQuery.exec())
			utils::throwQueryError(technologyQuery);

	}
}

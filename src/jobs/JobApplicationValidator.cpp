#include "JobApplicationValidator.hpp"

namespace {

	JobApplicationValidationResult validateValues(
		const QString& jobTitle,
		const QString& companyName,
		const QUrl& jobUrl,
		WorkFormat workFormat,
		JobStatus status,
		const QDate& appliedDate,
		bool hasCv)
	{
		JobApplicationValidationResult result;
		if (jobTitle.trimmed().isEmpty()) {
			result.fieldErrors_.insert(
				QStringLiteral("jobTitle"),
				QStringLiteral("Job title is required."));
		}
		if (companyName.trimmed().isEmpty()) {
			result.fieldErrors_.insert(
				QStringLiteral("companyName"),
				QStringLiteral("Company is required."));
		}
		if (!hasCv) {
			result.fieldErrors_.insert(
				QStringLiteral("cv"),
				QStringLiteral("Select a CV."));
		}
		if (!appliedDate.isValid()) {
			result.fieldErrors_.insert(
				QStringLiteral("appliedDate"),
				QStringLiteral("Use date format yyyy-MM-dd."));
		}
		if (status == JobStatus::Unknown) {
			result.fieldErrors_.insert(
				QStringLiteral("status"),
				QStringLiteral("Choose an allowed status."));
		}
		if (workFormat == WorkFormat::Unknown) {
			result.fieldErrors_.insert(
				QStringLiteral("workFormat"),
				QStringLiteral("Choose Remote, Hybrid, or On-site."));
		}

		if (jobUrl.isEmpty()) {
			const auto scheme = jobUrl.scheme();
			qDebug() << "scheme: " << scheme;
			if (!jobUrl.isValid()
				|| jobUrl.host().isEmpty()
				|| (scheme.compare(QStringLiteral("http"), Qt::CaseInsensitive) != 0
					&& scheme.compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0))
			{
				result.fieldErrors_.insert(
					QStringLiteral("jobUrl"),
					QStringLiteral("Use a valid HTTP or HTTPS URL."));
			}
		}
		return result;
	}

}

bool JobApplicationValidationResult::isValid() const
{
	return fieldErrors_.isEmpty();
}

QStringList JobApplicationValidationResult::messages() const
{
	QStringList result;
	result.reserve(fieldErrors_.size());
	for (auto it = fieldErrors_.cbegin(); it != fieldErrors_.cend(); ++it) {
		result.append(it.value().toString());
	}
	return result;
}

JobApplicationValidationResult JobApplicationValidator::validate(
	const NormalizedJobApplicationDraft& draft,
	const QUrl& selectedCvUrl)
{
	return validateValues(
		draft.jobTitle_,
		draft.companyName_,
		draft.jobUrl_,
		draft.workFormat_,
		draft.status_,
		draft.appliedDate_,
		!selectedCvUrl.isEmpty());
}

JobApplicationValidationResult JobApplicationValidator::validate(
	const JobApplication& application)
{
	return validateValues(
		application.jobTitle_,
		application.companyName_,
		application.jobUrl_,
		application.workFormat_,
		application.status_,
		application.appliedDate_,
		!application.cvId_.trimmed().isEmpty());
}

#ifndef JOBTRACKER_SRC_JOBS_JOBAPPLICATION_HPP
#define JOBTRACKER_SRC_JOBS_JOBAPPLICATION_HPP

#include <QDate>
#include <QDateTime>
#include <QLatin1StringView>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <array>
#include <cstddef>

// Closed domain choices stored by their existing schema-v3 text values.
enum class JobStatus
{
    Unknown,
    Applied,
    Interview,
    Offer,
    TestTask,
    Rejected
};

enum class WorkFormat
{
    Unknown,
    Unspecified,
    Remote,
    Hybrid,
    OnSite
};

JobStatus jobStatusFromString(const QString& value);
QString jobStatusToString(JobStatus status);
WorkFormat workFormatFromString(const QString& value);
QString workFormatToString(WorkFormat workFormat);

struct JobApplication
{
    QString id_;
    QString companyId_;
    QString companyName_;
    QString jobTitle_;
    QUrl jobUrl_;
    WorkFormat workFormat_ = WorkFormat::Unspecified;
    QString city_;
    QString salary_;
    JobStatus status_ = JobStatus::Unknown;
    QDate appliedDate_;
    QString nextStep_;
    QString cvId_;
    QString cvFileName_;
    QString description_;
    QString requirements_;
    QStringList techStack_;
    QString notes_;
    QDateTime createdAt_;
    QDateTime updatedAt_;
};

namespace jobs::detail {

	template<typename Enum>
	struct EnumText final
	{
		Enum value_;
		QLatin1StringView text_;
	};

	template<typename Enum, std::size_t Size>
	constexpr Enum enumFromString(
		const QString& value,
		const std::array<EnumText<Enum>, Size>& mappings,
		Enum fallback)
	{
		const auto normalized = value.trimmed();
		for (const auto& mapping : mappings) {
			if (normalized.compare(mapping.text_, Qt::CaseInsensitive) == 0) {
				return mapping.value_;
			}
		}
		return fallback;
	}

	template<typename Enum, std::size_t Size>
	constexpr QString enumToString(
		Enum value,
		const std::array<EnumText<Enum>, Size>& mappings)
	{
		for (const auto& mapping : mappings) {
			if (mapping.value_ == value) {
				return QString{ mapping.text_ };
			}
		}
		return {};
	}

}

#endif // JOBTRACKER_SRC_JOBS_JOBAPPLICATION_HPP

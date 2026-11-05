#include "JobApplication.hpp"

#include <QLatin1StringView>

#include <array>

namespace {

	using jobs::detail::EnumText;
	using jobs::detail::enumFromString;
	using jobs::detail::enumToString;

	constexpr std::array<EnumText<JobStatus>, 5> jobStatusMappings{
		EnumText{JobStatus::Applied, QLatin1StringView{"Applied"}},
		EnumText{JobStatus::Interview, QLatin1StringView{"Interview"}},
		EnumText{JobStatus::Offer, QLatin1StringView{"Offer"}},
		EnumText{JobStatus::TestTask, QLatin1StringView{"Test Task"}},
		EnumText{JobStatus::Rejected, QLatin1StringView{"Rejected"}},
	};

	constexpr std::array<EnumText<WorkFormat>, 3> workFormatMappings{ 
		EnumText{WorkFormat::Remote, QLatin1StringView{"Remote"}},
		EnumText{WorkFormat::Hybrid, QLatin1StringView{"Hybrid"}},
		EnumText{WorkFormat::OnSite, QLatin1StringView{"On-site"}},
	 };

} // namespace

JobStatus jobStatusFromString(const QString& value)
{
	return enumFromString(value, jobStatusMappings, JobStatus::Unknown);
}

QString jobStatusToString(JobStatus status)
{
	return enumToString(status, jobStatusMappings);
}

WorkFormat workFormatFromString(const QString& value)
{
	const auto normalized = value.trimmed();
	if (normalized.isEmpty()) {
		return WorkFormat::Unspecified;
	}
	return enumFromString(normalized, workFormatMappings, WorkFormat::Unknown);
}

QString workFormatToString(WorkFormat workFormat)
{
	return enumToString(workFormat, workFormatMappings);
}

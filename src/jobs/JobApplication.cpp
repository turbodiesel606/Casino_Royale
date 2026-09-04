#include "JobApplication.hpp"

#include <QLatin1StringView>

#include <array>

namespace {

	template<typename Enum>
	struct EnumText final
	{
		Enum value_;
		QLatin1StringView text_;
	};

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

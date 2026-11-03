#ifndef JOBTRACKER_SRC_LOGGING_LOGEVENT_HPP
#define JOBTRACKER_SRC_LOGGING_LOGEVENT_HPP

#include <QByteArray>
#include <QtGlobal>

#include <array>
#include <concepts>
#include <cstdint>
#include <type_traits>

enum class LogSeverity { Error, Warning, Info };
enum class LogComponent {
	Storage, Database, CvLibrary, Application, JobApplications,
	ApplicationModel, BackgroundWork, DataRemoval
};

enum class LogEventType
{
#define LOG_EVENT(name, severity, component, message) name,
#include "LogCatalog.inc"
#undef LOG_EVENT
	Count
};

enum class LogFact
{
#define LOG_FACT(name, text) name,
#include "CommonFacts.inc"
#include "PersistenceFacts.inc"
#include "StorageFacts.inc"
#include "WorkerFacts.inc"
#undef LOG_FACT
};

// Codex Comment: Queue values own only bounded diagnostic facts. Producer arguments
// follow the placeholder order of the event's descriptor in LogCatalog.inc.
struct LogValue final
{
	enum class Kind { Phrase, Number, Counts, JobUpdate };
	Kind kind_ = Kind::Phrase;
	LogFact fact_ = LogFact::Unknown;
	std::array<qint64, 5> numbers_{};

	constexpr LogValue() = default;
	constexpr LogValue(LogFact fact) : fact_{ fact } {}

	template<std::integral Number>
	constexpr LogValue(Number value) : kind_{ Kind::Number }, numbers_{ static_cast<qint64>(value) } {}

	template<std::integral... Numbers> requires (sizeof...(Numbers) <= 5)
		static constexpr LogValue counts(Numbers... numbers) noexcept
	{
		LogValue value;
		value.kind_ = Kind::Counts;
		value.numbers_ = { static_cast<qint64>(numbers)... };
		return value;
	}

	static constexpr LogValue jobUpdate(bool replacement, bool changed, LogFact disposition) noexcept
	{
		auto value = counts(replacement, changed);
		value.kind_ = Kind::JobUpdate;
		value.fact_ = disposition;
		return value;
	}
};

struct LogEvent final
{
	LogEventType type_;
	std::array<LogValue, 5> fields_{};
	quint64 operationId_ = 0;
};

static_assert(std::is_trivially_copyable_v<LogEvent>);

LogSeverity logSeverity(LogEventType type) noexcept;
QByteArray formatLogLine(const LogEvent& event, qint64 utcMilliseconds);

#endif // JOBTRACKER_SRC_LOGGING_LOGEVENT_HPP

#ifndef JOBTRACKER_SRC_LOGGING_LOGEVENT_HPP
#define JOBTRACKER_SRC_LOGGING_LOGEVENT_HPP

#include <QByteArray>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <string_view>
#include <type_traits>
#include <variant>

enum class LogSeverity { Error, Warning, Info };
enum class LogComponent {
	Storage, Database, CvLibrary, Application, JobApplications,
	ApplicationModel, BackgroundWork, DataRemoval, Count
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

// Codex Comment: Each grouped field has its own type. Required counts have no
// default constructor so omitting a member is a compile-time error.
struct LogCount final
{
    constexpr LogCount(qint64 value) noexcept : value_{value} {}
    qint64 value_;
};

struct LogFallbackCounts final { LogCount jobStatus_, workFormat_, requiredDate_, archiveTime_; };
struct LogRecoveryCounts final { LogCount restored_, removed_, quarantined_; };
struct LogPendingCounts final { LogCount active_, queued_; };
struct LogCompletionCounts final { LogCount processed_, unprocessed_; };
struct LogDeletionTotals final { LogCount committed_, failed_; };

struct LogRemovalCounts final
{
    qint64 deleted_ = 0;
    qint64 archived_ = 0;
    qint64 restored_ = 0;
    qint64 missing_ = 0;
    qint64 pending_ = 0;
};

struct LogJobUpdate final
{
    bool replacement_;
    bool relationshipChanged_;
    LogFact disposition_ = LogFact::None;
};

// Codex Comment: An unused slot is distinct from an explicitly absent detail (LogFact::None).
using LogValue = std::variant<std::monostate, LogFact, qint64, LogFallbackCounts, LogRecoveryCounts,
    LogPendingCounts, LogCompletionCounts, LogDeletionTotals, LogRemovalCounts, LogJobUpdate>;

namespace logging_detail {

// Codex Comment: Queue entries have fixed size for this build, sized to its widest catalog message.
inline constexpr auto logFieldCapacity = static_cast<std::size_t>(std::ranges::max(std::array{
#define LOG_EVENT(name, severity, component, message) std::ranges::count(std::string_view{message}, '{'),
#include "LogCatalog.inc"
#undef LOG_EVENT
}));

} // namespace logging_detail

struct LogEvent final
{
	LogEventType type_;
	std::array<LogValue, logging_detail::logFieldCapacity> fields_{};
	quint64 operationId_ = 0;
};

static_assert(std::is_trivially_copyable_v<LogEvent>);

LogSeverity logSeverity(LogEventType type) noexcept;
QByteArray formatLogLine(const LogEvent& event, qint64 utcMilliseconds);

#endif // JOBTRACKER_SRC_LOGGING_LOGEVENT_HPP

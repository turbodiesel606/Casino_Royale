#ifndef JOBTRACKER_SRC_LOGGING_APPLICATIONLOG_HPP
#define JOBTRACKER_SRC_LOGGING_APPLICATIONLOG_HPP

#include "LogEvent.hpp"
#include "LogInbox.hpp"

#include <QDateTime>

#include <concepts>
#include <thread>
#include <utility>

namespace logging_detail {

template<typename... Fields>
concept LogFieldArguments = (sizeof...(Fields) <= logFieldCapacity)
    && (std::convertible_to<Fields&&, LogValue> && ...);

}

namespace ApplicationLog {

template<typename... Fields>
    requires logging_detail::LogFieldArguments<Fields...>
void record(quint64 operationId, LogEventType type, Fields&&... fields) noexcept
{
    try {
        const auto timestamp = QDateTime::currentMSecsSinceEpoch();
        logging_detail::inbox().submit(logging_detail::LogEntry{
            .event_ = {type, {std::forward<Fields>(fields)...}, operationId},
            .utcMilliseconds_ = timestamp
        });
    } catch (...) {
        // Codex Comment: The caller can continue if field conversion or queue allocation fails.
    }
}

template<typename... Fields>
    requires logging_detail::LogFieldArguments<Fields...>
void record(LogEventType type, Fields&&... fields) noexcept
{
    ApplicationLog::record(quint64{0}, type, std::forward<Fields>(fields)...);
}

}

class ApplicationLogSession final
{
public:
    ApplicationLogSession() noexcept;
    ~ApplicationLogSession();

    ApplicationLogSession(const ApplicationLogSession&) = delete;
    ApplicationLogSession& operator=(const ApplicationLogSession&) = delete;

private:
    std::jthread consumer_;
};

#endif

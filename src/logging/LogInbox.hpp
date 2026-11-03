#ifndef JOBTRACKER_SRC_LOGGING_LOGINBOX_HPP
#define JOBTRACKER_SRC_LOGGING_LOGINBOX_HPP

#include "LogEvent.hpp"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace logging_detail {

struct LogEntry final
{
    LogEvent event_;
    qint64 utcMilliseconds_;
};

class LogInbox final
{
public:
    template<typename Consumer>
    void start(std::jthread& thread, Consumer&& consume)
    {
        std::lock_guard lock{mutex_};
        if (consumerAttached_)
            return;

        // Codex Comment: The consumer can first observe this state only after thread creation succeeds.
        thread = std::jthread{std::forward<Consumer>(consume)};
        consumerAttached_ = true;
        closing_ = false;
    }

    void submit(LogEntry entry);
    std::optional<LogEntry> tryTake();
    std::optional<LogEntry> waitAndTake();
    void closeAndJoin(std::jthread& thread);

private:
    std::optional<LogEntry> takeLocked();

    std::mutex mutex_;
    std::condition_variable ready_;
    std::deque<LogEntry> queue_;
    bool consumerAttached_ = false;
    bool closing_ = true;
};

}

#endif

#include "LogInbox.hpp"

namespace logging_detail {

void LogInbox::submit(LogEntry&& entry)
{
    {
        std::lock_guard lock{mutex_};
        if (closing_ || queue_.size() >= 4096)
            return;
        queue_.push_back(std::move(entry));
    }
    ready_.notify_one();
}

std::optional<LogEntry> LogInbox::takeLocked()
{
    if (queue_.empty())
        return std::nullopt;
    auto entry = std::move(queue_.front());
    queue_.pop_front();
    return entry;
}

std::optional<LogEntry> LogInbox::tryTake()
{
    std::lock_guard lock{mutex_};
    return takeLocked();
}

std::optional<LogEntry> LogInbox::waitAndTake()
{
    std::unique_lock lock{mutex_};
    ready_.wait(lock, [this] { return closing_ || !queue_.empty(); });
    return takeLocked();
}

void LogInbox::closeAndJoin(std::jthread& thread)
{
    if (!thread.joinable())
        return;
    {
        std::lock_guard lock{mutex_};
        closing_ = true;
    }
    ready_.notify_one();
    thread.join();
    std::lock_guard lock{mutex_};
    consumerAttached_ = false;
}

}

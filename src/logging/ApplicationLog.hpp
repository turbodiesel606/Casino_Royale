#ifndef JOBTRACKER_SRC_LOGGING_APPLICATIONLOG_HPP
#define JOBTRACKER_SRC_LOGGING_APPLICATIONLOG_HPP

#include "LogEvent.hpp"

#include <thread>

namespace ApplicationLog {
void record(LogEvent event) noexcept;
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

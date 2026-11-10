#include "ApplicationLog.hpp"

#include "LogFileWriter.hpp"
#include "LogInbox.hpp"

#include <QDir>
#include <QStandardPaths>

logging_detail::LogInbox& logging_detail::inbox()
{
    static LogInbox instance;
    return instance;
}

namespace {

void consume(logging_detail::LogInbox& source, const QString& directory)
{
    std::optional<logging_detail::LogFileWriter> writer;
    for (;;) {
        auto entry = source.tryTake();
        if (!entry) {
            if (writer)
                writer->flush();
            entry = source.waitAndTake();
            if (!entry)
                break;
        }
        try {
            if (!writer)
                writer.emplace(directory);
            writer->write(formatLogLine(entry->event_, entry->utcMilliseconds_),
                          logSeverity(entry->event_.type_) == LogSeverity::Error);
        } catch (...) {
            // Codex Comment: Discard only this record if allocating or encoding diagnostic data fails.
            writer.reset();
        }
    }
    if (writer)
        writer->flush();
}

}

ApplicationLogSession::ApplicationLogSession() noexcept
{
    try {
        const auto state = QStandardPaths::writableLocation(QStandardPaths::StateLocation);
        if (state.isEmpty())
            return;
        auto& source = logging_detail::inbox();
        source.start(consumer_, [&source, directory = QDir{state}.filePath(QStringLiteral("Logs"))] {
            consume(source, directory);
        });
    } catch (...) {
        // Codex Comment: Failed initialization leaves admission closed.
    }
}

ApplicationLogSession::~ApplicationLogSession()
{
    if (consumer_.joinable())
        logging_detail::inbox().closeAndJoin(consumer_);
}

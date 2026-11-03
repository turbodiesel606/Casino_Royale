#include "LogEvent.hpp"

#include <QDateTime>
#include <QTimeZone>

#include <array>
#include <string_view>

namespace {

using namespace std::string_view_literals;

struct Descriptor
{
    LogEventType type_;
    LogSeverity severity_;
    LogComponent component_;
    const char* componentName_;
    std::string_view message_;
};

constexpr std::array catalog{
#define LOG_EVENT(name, severity, component, message) \
    Descriptor{LogEventType::name, LogSeverity::severity, LogComponent::component, #component, message},
#include "LogCatalog.inc"
#undef LOG_EVENT
};

consteval bool validCatalog()
{
    std::array<int, 3> counts{};
    for (std::size_t i = 0; i < catalog.size(); ++i) {
        const auto& descriptor = catalog[i];
        if (static_cast<std::size_t>(descriptor.type_) != i || descriptor.message_.empty()
            || *descriptor.componentName_ == '\0')
            return false;
        ++counts[static_cast<std::size_t>(descriptor.severity_)];
        auto remaining = descriptor.message_;
        int fields = 0;
        while (!remaining.empty()) {
            const auto open = remaining.find('{');
            const auto close = remaining.find('}');
            if (open == remaining.npos && close == remaining.npos)
                break;
            if (open == remaining.npos || close == remaining.npos || close <= open + 1
                || ++fields > static_cast<int>(LogEvent{}.fields_.size()))
                return false;
            remaining.remove_prefix(close + 1);
        }
    }
    return counts == std::array{26, 12, 24}
        && catalog.size() == static_cast<std::size_t>(LogEventType::Count);
}
static_assert(validCatalog());

const char* phrase(LogFact fact)
{
    switch (fact) {
#define LOG_FACT(name, text) case LogFact::name: return text;
#include "CommonFacts.inc"
#include "PersistenceFacts.inc"
#include "StorageFacts.inc"
#include "WorkerFacts.inc"
#undef LOG_FACT
    }
    return "unknown";
}

const char* platform()
{
#if defined(Q_OS_WIN)
    return "Windows";
#elif defined(Q_OS_LINUX)
    return "Linux";
#elif defined(Q_OS_MACOS)
    return "macOS";
#else
    return "unknown platform";
#endif
}

QByteArray substitute(LogEventType type, std::string_view key, const LogValue& value)
{
    if (type == LogEventType::ApplicationStartupBegan)
        return key == "version"sv ? JOBTRACKER_LOG_VERSION : platform();

    if (type == LogEventType::CvSourceReadFailed && key == "stage"sv) {
        switch (value.fact_) {
        case LogFact::PersistSourceInspection: return "inspection";
        case LogFact::PersistSourceOpen: return "open";
        case LogFact::PersistSourceRead: return "read";
        case LogFact::PersistSourceBuffer: return "buffering";
        default: break;
        }
    }

    const auto& n = value.numbers_;
    const auto number = [&n](int i) { return QByteArray::number(n[i]); };
    const QByteArray basic = value.kind_ == LogValue::Kind::Number ? number(0) : QByteArray(phrase(value.fact_));
    if (key == "cleanupDetail"sv)
        return basic.isEmpty() ? QByteArray{} : ", file cleanup: " + basic;
    if (key == "cvDetail"sv)
        return basic.isEmpty() ? QByteArray{} : ", CV outcome: " + basic;
    if (key == "affectedCountDetail"sv)
        return basic.isEmpty() ? QByteArray{} : "Affected items: " + basic + ".";
    if (key == "repeatDetail"sv)
        return basic.isEmpty() ? QByteArray{} : "Repeated attempts: " + basic + ".";
    if (key == "fieldCounts"sv)
        return "job status " + number(0) + ", work format " + number(1)
            + ", required date/time " + number(2) + ", archive timestamp " + number(3);
    if (key == "recoveryCounts"sv)
        return "restored " + number(0) + ", removed " + number(1) + ", quarantined " + number(2);
    if (key == "pendingCounts"sv)
        return "active " + number(0) + ", queued " + number(1);
    if (key == "completionCounts"sv)
        return value.kind_ == LogValue::Kind::Counts
            ? "Processed: " + number(0) + ", unprocessed: " + number(1) + "." : QByteArray{};
    if (key == "committedOutcomeDetail"sv) {
        if (value.kind_ == LogValue::Kind::Counts)
            return "Deleted: " + number(0) + ", archived: " + number(1) + ", restored: " + number(2)
                + ", already missing files: " + number(3) + ", pending cleanup: " + number(4) + ".";
        if (value.kind_ == LogValue::Kind::JobUpdate) {
            QByteArray detail = "CV replacement: ";
            detail += n[0] ? "yes" : "no";
            detail += ", relationship changed: ";
            detail += n[1] ? "yes" : "no";
            if (value.fact_ != LogFact::None)
                detail += ", CV outcome: " + basic;
            return detail + ".";
        }
        if (value.kind_ == LogValue::Kind::Number)
            return "Committed items: " + basic + ".";
        return basic.isEmpty() ? QByteArray{} : "CV outcome: " + basic + ".";
    }
    return basic;
}

} // namespace

LogSeverity logSeverity(LogEventType type) noexcept
{
    const auto index = static_cast<std::size_t>(type);
    return index < catalog.size() ? catalog[index].severity_ : LogSeverity::Error;
}

QByteArray formatLogLine(const LogEvent& event, qint64 utcMilliseconds)
{
    const auto index = static_cast<std::size_t>(event.type_);
    if (index >= catalog.size())
        return {};
    const auto& descriptor = catalog[index];
    const auto utc = QDateTime::fromMSecsSinceEpoch(utcMilliseconds, QTimeZone::utc());
    auto line = utc.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")).toLatin1();
    constexpr std::array severityNames{"ERROR", "WARNING", "INFO"};
    line += " | ";
    line += severityNames[static_cast<std::size_t>(descriptor.severity_)];
    line += " | ";
    line += descriptor.componentName_;
    line += " | Message: ";

    auto remaining = descriptor.message_;
    std::size_t field = 0;
    for (auto open = remaining.find('{'); open != remaining.npos; open = remaining.find('{')) {
        line.append(remaining.data(), static_cast<qsizetype>(open));
        const auto close = remaining.find('}', open);
        line += substitute(event.type_, remaining.substr(open + 1, close - open - 1), event.fields_[field++]);
        remaining.remove_prefix(close + 1);
    }
    line.append(remaining.data(), static_cast<qsizetype>(remaining.size()));
    if (line.endsWith(' '))
        line.chop(1);
    if (event.operationId_ != 0) {
        line.chop(1);
        line += " (operation " + QByteArray::number(event.operationId_) + ").";
    }
    line += '\n';
    return line;
}

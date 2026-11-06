#include "LogEvent.hpp"

#include <QDateTime>
#include <QTimeZone>

#include <algorithm>
#include <optional>
#include <string_view>

namespace {

using namespace std::string_view_literals;

enum class Form {
    Fact, Number, KnownNumber, SourceStage, AffectedDetail, RepeatDetail, CleanupDetail, CvDetail,
    Fallbacks, Recovery, Pending, Completion, Outcome, DeletionTotals, AppVersion, Platform
};

struct NamedForm
{
    std::string_view name_;
    Form form_;
};

constexpr std::array forms{
    NamedForm{"fact", Form::Fact},             NamedForm{"number", Form::Number},
    NamedForm{"knownNumber", Form::KnownNumber}, NamedForm{"sourceStage", Form::SourceStage},
    NamedForm{"affectedDetail", Form::AffectedDetail}, NamedForm{"repeatDetail", Form::RepeatDetail},
    NamedForm{"cleanupDetail", Form::CleanupDetail}, NamedForm{"cvDetail", Form::CvDetail},
    NamedForm{"fallbacks", Form::Fallbacks},   NamedForm{"recovery", Form::Recovery},
    NamedForm{"pending", Form::Pending},       NamedForm{"completion", Form::Completion},
    NamedForm{"outcome", Form::Outcome},       NamedForm{"deletionTotals", Form::DeletionTotals},
    NamedForm{"appVersion", Form::AppVersion}, NamedForm{"platform", Form::Platform}
};

constexpr bool generated(Form form)
{
    return form == Form::AppVersion || form == Form::Platform;
}

constexpr const char* severityName(LogSeverity severity)
{
    switch (severity) {
    case LogSeverity::Error: return "ERROR";
    case LogSeverity::Warning: return "WARNING";
    case LogSeverity::Info: return "INFO";
    }
    return nullptr;
}

struct Field
{
    std::size_t open_;
    std::size_t close_;
    Form form_;
};

struct Descriptor
{
    LogEventType type_;
    LogSeverity severity_;
    LogComponent component_;
    std::string_view componentName_;
    std::string_view message_;
    std::array<Field, logging_detail::logFieldCapacity> fields_{};
    std::size_t fieldCount_ = 0;
};

consteval Descriptor describe(LogEventType type, LogSeverity severity, LogComponent component,
                              std::string_view componentName, std::string_view message)
{
    Descriptor result{type, severity, component, componentName, message};
    if (!severityName(severity) || component < LogComponent::Storage || component >= LogComponent::Count
        || componentName.empty() || message.empty() || message.find_first_of("\r\n|") != message.npos)
        throw "Invalid log descriptor";

    std::size_t pos = 0;
    while ((pos = message.find_first_of("{}", pos)) != message.npos) {
        const auto close = message.find_first_of("{}", pos + 1);
        if (message[pos] != '{' || close == message.npos || message[close] != '}'
            || result.fieldCount_ == result.fields_.size())
            throw "Invalid log placeholder";
        const auto token = message.substr(pos + 1, close - pos - 1);
        const auto colon = token.find(':');
        const auto name = token.substr(0, colon);
        if (name.empty() || name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != name.npos)
            throw "Invalid log field name";
        const auto formName = colon == token.npos ? "fact"sv : token.substr(colon + 1);
        const auto form = std::ranges::find(forms, formName, &NamedForm::name_);
        if (form == forms.end())
            throw "Unknown log payload form";
        result.fields_[result.fieldCount_++] = {pos, close, form->form_};
        pos = close + 1;
    }
    if (!message.ends_with('.')) {
        if (!message.ends_with('}') || !result.fieldCount_)
            throw "Log messages must end with a sentence";
        const auto& last = result.fields_[result.fieldCount_ - 1];
        const bool optionalSentence = last.form_ == Form::AffectedDetail || last.form_ == Form::RepeatDetail
            || last.form_ == Form::Completion || last.form_ == Form::Outcome;
        if (last.form_ != Form::DeletionTotals
            && !(optionalSentence && message.substr(0, last.open_).ends_with(". ")))
            throw "A trailing detail must complete the sentence even when omitted";
    }
    return result;
}

constexpr std::array catalog{
#define LOG_EVENT(name, severity, component, message) \
    describe(LogEventType::name, LogSeverity::severity, LogComponent::component, #component, message),
#include "LogCatalog.inc"
#undef LOG_EVENT
};

static_assert(catalog.size() == static_cast<std::size_t>(LogEventType::Count));
static_assert([] {
    for (std::size_t i = 0; i < catalog.size(); ++i)
        if (static_cast<std::size_t>(catalog[i].type_) != i)
            return false;
    return true;
}());

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

QByteArray number(qint64 value) { return QByteArray::number(value); }
QByteArray number(LogCount count) { return number(count.value_); }

std::optional<QByteArray> substitute(Form form, const LogValue& value)
{
    const auto* fact = std::get_if<LogFact>(&value);
    const auto* n = std::get_if<qint64>(&value);
    const bool omitted = fact && *fact == LogFact::None;
    switch (form) {
    case Form::AppVersion: return QByteArray{JOBTRACKER_LOG_VERSION};
    case Form::Platform: return QByteArray{platform()};
    case Form::Fact:
        if (fact && !omitted) return QByteArray{phrase(*fact)};
        break;
    case Form::Number:
        if (n) return number(*n);
        break;
    case Form::KnownNumber:
        if (n) return number(*n);
        if (fact && *fact == LogFact::Unknown) return QByteArray{phrase(*fact)};
        break;
    case Form::SourceStage:
        if (!fact) break;
        switch (*fact) {
        case LogFact::PersistSourceInspection: return QByteArray{"inspection"};
        case LogFact::PersistSourceOpen: return QByteArray{"open"};
        case LogFact::PersistSourceRead: return QByteArray{"read"};
        case LogFact::PersistSourceBuffer: return QByteArray{"buffering"};
        default: break;
        }
        break;
    case Form::AffectedDetail:
    case Form::RepeatDetail:
        if (omitted) return QByteArray{};
        if (n) return (form == Form::AffectedDetail ? "Affected items: " : "Repeated attempts: ") + number(*n) + ".";
        break;
    case Form::CleanupDetail:
    case Form::CvDetail:
        if (omitted) return QByteArray{};
        if (fact) return QByteArray{form == Form::CleanupDetail ? ", file cleanup: " : ", CV outcome: "} + phrase(*fact);
        break;
    case Form::Fallbacks:
        if (const auto* c = std::get_if<LogFallbackCounts>(&value))
            return "job status " + number(c->jobStatus_) + ", work format " + number(c->workFormat_)
                + ", required date/time " + number(c->requiredDate_) + ", archive timestamp " + number(c->archiveTime_);
        break;
    case Form::Recovery:
        if (const auto* c = std::get_if<LogRecoveryCounts>(&value))
            return "restored " + number(c->restored_) + ", removed " + number(c->removed_) + ", quarantined " + number(c->quarantined_);
        break;
    case Form::Pending:
        if (const auto* c = std::get_if<LogPendingCounts>(&value))
            return "active " + number(c->active_) + ", queued " + number(c->queued_);
        break;
    case Form::Completion:
        if (omitted) return QByteArray{};
        if (const auto* c = std::get_if<LogCompletionCounts>(&value))
            return "Processed: " + number(c->processed_) + ", unprocessed: " + number(c->unprocessed_) + ".";
        break;
    case Form::DeletionTotals:
        if (const auto* c = std::get_if<LogDeletionTotals>(&value))
            return "Committed: " + number(c->committed_) + ", failed: " + number(c->failed_) + ".";
        break;
    case Form::Outcome:
        if (const auto* c = std::get_if<LogRemovalCounts>(&value))
            return "Deleted: " + number(c->deleted_) + ", archived: " + number(c->archived_) + ", restored: " + number(c->restored_)
                + ", already missing files: " + number(c->missing_) + ", pending cleanup: " + number(c->pending_) + ".";
        if (const auto* update = std::get_if<LogJobUpdate>(&value)) {
            QByteArray detail = update->replacement_ ? "CV replacement: yes" : "CV replacement: no";
            detail += update->relationshipChanged_ ? ", relationship changed: yes" : ", relationship changed: no";
            if (update->disposition_ != LogFact::None)
                detail += QByteArray{", CV outcome: "} + phrase(update->disposition_);
            return detail + ".";
        }
        if (omitted) return QByteArray{};
        if (fact) return QByteArray{"CV outcome: "} + phrase(*fact) + ".";
        if (n) return "Committed items: " + number(*n) + ".";
        break;
    }
    return std::nullopt;
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
    line += " | ";
    line += severityName(descriptor.severity_);
    line += " | ";
    line.append(descriptor.componentName_.data(), static_cast<qsizetype>(descriptor.componentName_.size()));
    line += " | Message: ";

    std::size_t pos = 0;
    std::size_t payload = 0;
    constexpr LogValue generatedField{};
    for (std::size_t i = 0; i < descriptor.fieldCount_; ++i) {
        const auto& field = descriptor.fields_[i];
        line.append(descriptor.message_.data() + pos, static_cast<qsizetype>(field.open_ - pos));
        const auto& value = generated(field.form_) ? generatedField : event.fields_[payload++];
        const auto detail = substitute(field.form_, value);
        if (!detail) return {};
        line += *detail;
        pos = field.close_ + 1;
    }
    for (; payload < event.fields_.size(); ++payload)
        if (!std::holds_alternative<std::monostate>(event.fields_[payload])) return {};
    line.append(descriptor.message_.data() + pos, static_cast<qsizetype>(descriptor.message_.size() - pos));
    if (line.endsWith(' '))
        line.chop(1);
    if (event.operationId_ != 0) {
        line.chop(1);
        line += " (operation " + QByteArray::number(event.operationId_) + ").";
    }
    line += '\n';
    return line;
}

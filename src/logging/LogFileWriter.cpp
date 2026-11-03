#include "LogFileWriter.hpp"

#include <QFileInfo>

#include <iterator>
#include <limits>
#include <map>
#include <optional>

namespace logging_detail {
namespace {

constexpr qint64 segmentLimit = 5 * 1024 * 1024;
constexpr quint64 retentionLimit = 50 * 1024 * 1024;

QString segmentName(quint64 number)
{
    return QStringLiteral("jobtracker-%1.log").arg(number, 6, 10, QLatin1Char{'0'});
}

std::optional<quint64> segmentNumber(const QString& name)
{
    constexpr auto prefix = QLatin1StringView{"jobtracker-"};
    constexpr auto suffix = QLatin1StringView{".log"};
    if (!name.startsWith(prefix) || !name.endsWith(suffix))
        return std::nullopt;
    bool ok = false;
    const auto digits = QStringView{name}.sliced(prefix.size(),
        name.size() - prefix.size() - suffix.size());
    const auto number = digits.toULongLong(&ok);
    if (!ok || number == 0 || name != segmentName(number))
        return std::nullopt;
    return number;
}

struct Segments
{
    std::map<quint64, qint64> sizes_;
    quint64 total_ = 0;
};

Segments scanSegments(QDir& directory)
{
    Segments segments;
    directory.refresh();
    const auto files = directory.entryInfoList({QStringLiteral("jobtracker-*.log")},
        QDir::Files | QDir::Hidden | QDir::System | QDir::NoSymLinks, QDir::Unsorted);
    for (const auto& info : files) {
        if (!info.isFile() || info.isSymLink())
            continue;
        if (const auto number = segmentNumber(info.fileName())) {
            // Codex Comment: Directory enumeration on Windows can lag a recently closed file.
            const auto size = QFileInfo{info.absoluteFilePath()}.size();
            segments.sizes_.emplace(*number, size);
            segments.total_ += static_cast<quint64>(size);
        }
    }
    return segments;
}

QString canonicalLogDirectory(const QDir& directory)
{
    const QFileInfo info{directory.path()};
    if (info.isSymLink() || info.isJunction() || !info.isDir())
        return {};
    return info.canonicalFilePath();
}

}

LogFileWriter::LogFileWriter(const QString& directory)
    : directory_{directory}
{
}

bool LogFileWriter::resume(quint64 number, qint64 bytes)
{
    const auto path = directory_.filePath(segmentName(number));
    const QFileInfo info{path};
    if (info.isSymLink() || !info.isFile())
        return false;
    file_.setFileName(path);
    if (!file_.open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::ExistingOnly))
        return false;
    const auto length = file_.size();
    char tail = 0;
    if (length < 0 || length > segmentLimit - bytes
        || (length && (!file_.seek(length - 1) || !file_.getChar(&tail) || tail != '\n'))) {
        file_.close();
        return false;
    }
    activeBytes_ = length;
    activeNumber_ = number;
    return true;
}

bool LogFileWriter::openFor(qint64 bytes)
{
    if (file_.isOpen()) {
        if (activeBytes_ <= segmentLimit - bytes)
            return true;
        flush();
        file_.close();
    }

    if (!directory_.mkpath(QStringLiteral(".")))
        return false;
    canonicalDirectory_ = canonicalLogDirectory(directory_);
    if (canonicalDirectory_.isEmpty())
        return false;
    const auto segments = scanSegments(directory_);
    closedBytes_ = segments.total_;
    quint64 latest = 0;
    if (!segments.sizes_.empty()) {
        const auto last = std::prev(segments.sizes_.end());
        latest = last->first;
        if (resume(latest, bytes)) {
            closedBytes_ -= static_cast<quint64>(last->second);
            return true;
        }
    }
    while (latest < std::numeric_limits<quint64>::max()) {
        const auto path = directory_.filePath(segmentName(++latest));
        const QFileInfo candidate{path};
        if (candidate.isSymLink() || candidate.exists())
            continue;
        file_.setFileName(path);
        if (!file_.open(QIODevice::WriteOnly | QIODevice::NewOnly))
            return false;
        activeNumber_ = latest;
        activeBytes_ = 0;
        return true;
    }
    return false;
}

void LogFileWriter::prune()
{
    if (closedBytes_ + static_cast<quint64>(activeBytes_) <= retentionLimit)
        return;
    // Codex Comment: Buffered bytes can overstate usage when a flush fails. Only
    // the fresh on-disk inventory may authorize deleting a closed segment.
    flush();
    if (canonicalLogDirectory(directory_) != canonicalDirectory_)
        return;
    const auto segments = scanSegments(directory_);
    const auto active = segments.sizes_.find(activeNumber_);
    if (active == segments.sizes_.end())
        return;
    // Codex Comment: The flushed handle has the current length even when Windows
    // still reports a cached directory-entry length for the open segment.
    const auto actualSize = file_.isOpen() ? file_.size() : active->second;
    if (actualSize < 0)
        return;
    activeBytes_ = actualSize;
    closedBytes_ = segments.total_ - static_cast<quint64>(active->second);
    for (const auto& [number, size] : segments.sizes_) {
        if (closedBytes_ + static_cast<quint64>(activeBytes_) <= retentionLimit)
            break;
        if (number == activeNumber_ || size <= 0)
            continue;
        if (canonicalLogDirectory(directory_) != canonicalDirectory_)
            return;
        const auto path = directory_.filePath(segmentName(number));
        const QFileInfo info{path};
        if (info.isSymLink() || !info.isFile() || info.size() != size)
            return;
        if (QFile::remove(path))
            closedBytes_ -= static_cast<quint64>(size);
    }
}

void LogFileWriter::write(QByteArrayView line, bool error)
{
    if (line.isEmpty() || line.size() > segmentLimit || !openFor(line.size()))
        return;

    qsizetype offset = 0;
    while (offset < line.size()) {
        const auto written = file_.write(line.data() + offset, line.size() - offset);
        if (written <= 0) {
            // Codex Comment: The next open rechecks the actual tail before reusing this segment.
            flush();
            file_.close();
            prune();
            return;
        }
        offset += written;
        activeBytes_ += written;
        if (!dirty_) {
            dirty_ = true;
            flushDeadline_ = std::chrono::steady_clock::now() + std::chrono::seconds{1};
        }
    }
    if (error || std::chrono::steady_clock::now() >= flushDeadline_)
        flush();
    prune();
}

void LogFileWriter::flush() noexcept
{
    if (!dirty_)
        return;
    dirty_ = false;
    try {
        if (!file_.flush())
            file_.close();
    } catch (...) {
        file_.close();
    }
}

}

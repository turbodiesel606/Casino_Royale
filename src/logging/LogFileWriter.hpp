#ifndef JOBTRACKER_SRC_LOGGING_LOGFILEWRITER_HPP
#define JOBTRACKER_SRC_LOGGING_LOGFILEWRITER_HPP

#include <QByteArrayView>
#include <QDir>
#include <QFile>

#include <chrono>

namespace logging_detail {

// Codex Comment: One consumer owns the file and its closed-segment inventory.
class LogFileWriter final
{
public:
    explicit LogFileWriter(const QString& directory);
    void write(QByteArrayView line, bool error);
    void flush() noexcept;

private:
    bool openFor(qint64 bytes);
    bool resume(quint64 number, qint64 bytes);
    void prune();

    QDir directory_;
    QString canonicalDirectory_;
    QFile file_;
    quint64 activeNumber_ = 0;
    qint64 activeBytes_ = 0;
    quint64 closedBytes_ = 0;
    std::chrono::steady_clock::time_point flushDeadline_;
    bool dirty_ = false;
};

}

#endif

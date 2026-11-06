#include "logging/ApplicationLog.hpp"
#include "logging/LogFileWriter.hpp"
#include "logging/LogInbox.hpp"
#include "../support/LogCapture.hpp"

#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <array>
#include <barrier>
#include <filesystem>
#include <latch>
#include <vector>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {

constexpr qint64 fiveMiB = 5 * 1024 * 1024;
using logging_detail::LogFileWriter;
using logging_detail::LogInbox;

bool seedSegment(const QString& path, qint64 size)
{
    QFile file{path};
    return file.open(QIODevice::WriteOnly | QIODevice::NewOnly)
        && file.resize(size) && file.seek(size - 1) && file.putChar('\n') && file.flush();
}

QByteArray contents(const QString& path)
{
    QFile file{path};
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
}

QString segmentPath(const QString& directory, int number)
{
    return QDir{directory}.filePath(QStringLiteral("jobtracker-%1.log").arg(number, 6, 10, QLatin1Char{'0'}));
}

bool seedNearlyFullDirectory(const QString& path)
{
    if (!QDir{}.mkpath(path))
        return false;
    for (int i = 1; i <= 10; ++i) {
        if (!seedSegment(segmentPath(path, i), i == 1 ? fiveMiB - 4 : fiveMiB))
            return false;
    }
    return seedSegment(segmentPath(path, 11), 2);
}

}

class LogWriterTest final : public QObject
{
    Q_OBJECT

private slots:
    void capacityDropsIncomingRecordsAtEverySeverity();
    void admissionClosureDrainsAcceptedPrefix();
    void concurrentProducersWriteCompleteOrderedLines();
    void restartAppendsAndRotatesAtTheExactByteLimit();
    void incompleteTailIsRetired();
    void prunesOnlyAboveBudgetAndOnlyOldestRecognizedFiles();
    void rechecksPhysicalUsageBeforeDeleting();
    void replacedActiveSegmentDoesNotOverstateUsage();
    void skipsSymlinks();
    void replacedLogDirectoryCannotRedirectPruning();
    void retriesWhenDirectoryBecomesUsable();
};

void LogWriterTest::capacityDropsIncomingRecordsAtEverySeverity()
{
    LogInbox inbox;
    std::jthread consumer;
    std::latch releaseConsumer{1};
    std::vector<qint64> accepted;
    inbox.submit({{LogEventType::ApplicationStarted}, -1});
    inbox.start(consumer, [&] {
        releaseConsumer.wait();
        while (auto item = inbox.waitAndTake())
            accepted.push_back(item->utcMilliseconds_);
    });
    for (int i = 0; i < 4096; ++i)
        inbox.submit({{LogEventType::JobApplicationsDeleted}, i});
    for (auto type : {LogEventType::ApplicationStarted, LogEventType::JobUpdateAlreadyPending,
                      LogEventType::CvFavoriteSaveFailed})
        inbox.submit({{type}, -2});
    releaseConsumer.count_down();
    inbox.closeAndJoin(consumer);
    QCOMPARE(accepted.size(), size_t{4096});
    for (int i = 0; i < 4096; ++i)
        QCOMPARE(accepted[i], i);
}

void LogWriterTest::admissionClosureDrainsAcceptedPrefix()
{
    LogInbox inbox;
    std::jthread consumer;
    std::vector<qint64> processed;
    inbox.start(consumer, [&] {
        while (auto item = inbox.waitAndTake())
            processed.push_back(item->utcMilliseconds_);
    });
    std::barrier race{2};
    std::jthread producer{[&] {
        inbox.submit({{LogEventType::JobApplicationsDeleted}, 0});
        race.arrive_and_wait();
        for (int i = 1; i <= 2000; ++i)
            inbox.submit({{LogEventType::JobApplicationsDeleted}, i});
    }};
    race.arrive_and_wait();
    inbox.closeAndJoin(consumer);
    producer.join();
    inbox.submit({{LogEventType::ApplicationStarted}, -1});
    QVERIFY(!inbox.tryTake());
    QVERIFY(!processed.empty());
    for (size_t i = 0; i < processed.size(); ++i)
        QCOMPARE(processed[i], static_cast<qint64>(i));
}

void LogWriterTest::concurrentProducersWriteCompleteOrderedLines()
{
    testsupport::LogCapture capture;
    QVERIFY(capture.isValid());
    ApplicationLog::record({LogEventType::ApplicationStarted});
    {
        ApplicationLogSession idle;
    }
    QVERIFY(capture.contents().isEmpty());
    constexpr int producerCount = 4;
    constexpr int perProducer = 100;
    {
        ApplicationLogSession session;
        ApplicationLog::record({LogEventType::ApplicationStarted});
        QTRY_VERIFY_WITH_TIMEOUT(capture.contents().endsWith("available.\n"), 5000);
        std::array<std::jthread, producerCount> producers;
        for (int i = 0; i < producerCount; ++i) {
            producers[i] = std::jthread{[i] {
                for (int n = 0; n < perProducer; ++n)
                    ApplicationLog::record({LogEventType::JobApplicationsDeleted, {1 + i * perProducer + n}});
            }};
        }
    }
    ApplicationLog::record({LogEventType::ControlledShutdownCompleted});
    auto lines = capture.contents().split('\n');
    QVERIFY(lines.takeFirst().endsWith("available."));
    QVERIFY(lines.takeLast().isEmpty());
    QCOMPARE(lines.size(), producerCount * perProducer);
    std::array<int, producerCount> seen{};
    const QRegularExpression linePattern{QStringLiteral(
        R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3} \| INFO \| JobApplications \| Message: Job deletion committed for (\d+) applications\.$)")};
    for (const auto& line : lines) {
        const auto match = linePattern.match(QString::fromUtf8(line));
        QVERIFY2(match.hasMatch(), line.constData());
        const int number = match.captured(1).toInt() - 1;
        QVERIFY(number >= 0 && number < producerCount * perProducer);
        QCOMPARE(number % perProducer, seen[number / perProducer]++);
    }
    for (const auto count : seen)
        QCOMPARE(count, perProducer);
}

void LogWriterTest::restartAppendsAndRotatesAtTheExactByteLimit()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const auto first = root.filePath(QStringLiteral("jobtracker-000001.log"));
    const auto second = root.filePath(QStringLiteral("jobtracker-000002.log"));
    QVERIFY(seedSegment(first, fiveMiB - 4));
    {
        LogFileWriter writer{root.path()};
        writer.write("a\n", true);
        QCOMPARE(QFileInfo{first}.size(), fiveMiB - 2);
    }
    {
        LogFileWriter writer{root.path()};
        writer.write("b\n", true);
        QCOMPARE(QFileInfo{first}.size(), fiveMiB);
        QVERIFY(!QFileInfo::exists(second));
        writer.write("c\n", true);
    }
    QVERIFY(contents(first).endsWith("\na\nb\n"));
    QCOMPARE(contents(second), QByteArray{"c\n"});
}

void LogWriterTest::incompleteTailIsRetired()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    QFile first{root.filePath(QStringLiteral("jobtracker-000001.log"))};
    QVERIFY(first.open(QIODevice::WriteOnly));
    QCOMPARE(first.write("whole\npartial"), 13);
    first.close();
    LogFileWriter writer{root.path()};
    writer.write("next\n", true);
    QCOMPARE(contents(first.fileName()), QByteArray{"whole\npartial"});
    QCOMPARE(contents(root.filePath(QStringLiteral("jobtracker-000002.log"))), QByteArray{"next\n"});
}

void LogWriterTest::prunesOnlyAboveBudgetAndOnlyOldestRecognizedFiles()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const auto segment = [&](int i) { return segmentPath(root.path(), i); };
    for (int i = 1; i <= 10; ++i)
        QVERIFY(seedSegment(segment(i), i == 10 ? fiveMiB - 2 : fiveMiB));
    const auto unrelated = root.filePath(QStringLiteral("jobtracker-not-a-number.log"));
    const auto noncanonical = root.filePath(QStringLiteral("jobtracker-01.log"));
    QVERIFY(seedSegment(unrelated, 1024));
    QVERIFY(seedSegment(noncanonical, 1024));
    LogFileWriter writer{root.path()};
    writer.write("x\n", true);
    QVERIFY(QFileInfo::exists(segment(1)));
    writer.write("y\n", true);
    QVERIFY(!QFileInfo::exists(segment(1)));
    for (int i = 2; i <= 10; ++i)
        QCOMPARE(QFileInfo{segment(i)}.size(), fiveMiB);
    QCOMPARE(contents(segment(11)), QByteArray{"y\n"});
    QCOMPARE(QFileInfo{unrelated}.size(), 1024);
    QCOMPARE(QFileInfo{noncanonical}.size(), 1024);
}

void LogWriterTest::rechecksPhysicalUsageBeforeDeleting()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    QVERIFY(seedNearlyFullDirectory(root.path()));
    LogFileWriter writer{root.path()};
    writer.write("x\n", true);
    const auto first = segmentPath(root.path(), 1);
    const auto second = segmentPath(root.path(), 2);
    QVERIFY(QFile::resize(second, fiveMiB - 10));
    writer.write("y\n", false);
    QCOMPARE(QFileInfo{first}.size(), fiveMiB - 4);
    QCOMPARE(QFileInfo{second}.size(), fiveMiB - 10);
    QCOMPARE(QFileInfo{segmentPath(root.path(), 11)}.size(), 6);
}

void LogWriterTest::replacedActiveSegmentDoesNotOverstateUsage()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    QVERIFY(seedNearlyFullDirectory(root.path()));
    const auto first = segmentPath(root.path(), 1);
    const auto active = segmentPath(root.path(), 11);
    const auto moved = root.filePath(QStringLiteral("external.log"));
    LogFileWriter writer{root.path()};
    writer.write("x\n", true);
    QCOMPARE(QFileInfo{first}.size(), fiveMiB - 4);
    QCOMPARE(QFileInfo{active}.size(), 4);
    std::error_code error;
    std::filesystem::rename(QFileInfo{active}.filesystemFilePath(), QFileInfo{moved}.filesystemFilePath(), error);
#ifdef Q_OS_WIN
    if (error == std::errc::permission_denied || error == std::errc::device_or_resource_busy
        || error.value() == ERROR_SHARING_VIOLATION)
        QSKIP("Windows file sharing prevents renaming this open log segment.");
#endif
    QVERIFY2(!error, error.message().c_str());
    QVERIFY(seedSegment(active, 2));
    writer.write("y\n", true);
    QCOMPARE(QFileInfo{first}.size(), fiveMiB - 4);
    QCOMPARE(QFileInfo{active}.size(), 2);
    QCOMPARE(QFileInfo{moved}.size(), 6);
    writer.write("z\n", true);
    QCOMPARE(contents(active).right(2), QByteArray{"z\n"});
    QCOMPARE(QFileInfo{first}.size(), fiveMiB - 4);
}

void LogWriterTest::skipsSymlinks()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const auto target = root.filePath(QStringLiteral("unrelated.txt"));
    const auto link = root.filePath(QStringLiteral("jobtracker-000001.log"));
    QVERIFY(seedSegment(target, 1));
    std::error_code error;
    std::filesystem::create_symlink(QFileInfo{target}.filesystemFilePath(),
                                    QFileInfo{link}.filesystemFilePath(), error);
    if (error)
        QSKIP("This account cannot create native symbolic links.");
    LogFileWriter writer{root.path()};
    writer.write("next\n", true);
    QVERIFY(QFileInfo{link}.isSymLink());
    QCOMPARE(contents(target), QByteArray{"\n"});
    QCOMPARE(contents(root.filePath(QStringLiteral("jobtracker-000002.log"))), QByteArray{"next\n"});
}

void LogWriterTest::replacedLogDirectoryCannotRedirectPruning()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const auto logs = root.filePath(QStringLiteral("Logs"));
    const auto moved = root.filePath(QStringLiteral("Moved"));
    const auto foreign = root.filePath(QStringLiteral("Foreign"));
    QVERIFY(seedNearlyFullDirectory(logs));
    QVERIFY(seedNearlyFullDirectory(foreign));
    const auto protectedFile = segmentPath(foreign, 1);
    QVERIFY(QFile::resize(segmentPath(foreign, 11), 8));
    LogFileWriter writer{logs};
    writer.write("x\n", true);
    if (!QDir{}.rename(logs, moved))
        QSKIP("The filesystem prevents replacing an open log directory.");
    std::error_code error;
    std::filesystem::create_directory_symlink(QFileInfo{foreign}.filesystemFilePath(),
                                              QFileInfo{logs}.filesystemFilePath(), error);
    if (error)
        QSKIP("This account cannot create native directory symbolic links.");
    writer.write("y\n", true);
    QCOMPARE(QFileInfo{protectedFile}.size(), fiveMiB - 4);
}

void LogWriterTest::retriesWhenDirectoryBecomesUsable()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    const auto logs = root.filePath(QStringLiteral("Logs"));
    QVERIFY(seedSegment(logs, 1));
    LogFileWriter writer{logs};
    writer.write("lost\n", true);
    QCOMPARE(contents(logs), QByteArray{"\n"});
    QVERIFY(QFile::remove(logs));
    writer.write("saved\n", true);
    QCOMPARE(contents(QDir{logs}.filePath(QStringLiteral("jobtracker-000001.log"))), QByteArray{"saved\n"});
}

QTEST_GUILESS_MAIN(LogWriterTest)

#include "LogWriterTest.moc"

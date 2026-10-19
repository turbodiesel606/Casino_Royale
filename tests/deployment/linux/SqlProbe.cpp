#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>
#include <QVariant>

// Codex Comment: Checks only the distributed SQL runtime, without App, GUI or user databases.
int main(int argc, char* argv[])
{
    QCoreApplication application{argc, argv};
    if (application.arguments().size() != 2) {
        qCritical() << "Usage: JobTrackerSqlProbe <expected-private-runtime-prefix>";
        return 1;
    }
    const QDir prefix{QFileInfo{application.arguments().at(1)}.canonicalFilePath()};
    if (prefix.path().isEmpty() || prefix.isRoot() ||
        QFileInfo{QCoreApplication::applicationDirPath() + QStringLiteral("/..")}
                .canonicalFilePath() != prefix.path() ||
        QFileInfo{QLibraryInfo::path(QLibraryInfo::PrefixPath)}.canonicalFilePath() != prefix.path() ||
        QString::fromLatin1(qVersion()) != QStringLiteral("6.10.3")) {
        qCritical() << "Unexpected executable location, qt.conf prefix or Qt version";
        return 2;
    }
    const auto insidePrefix = [&prefix](const QString& path) {
        const auto canonical = QFileInfo{path}.canonicalFilePath();
        return !canonical.isEmpty() && canonical.startsWith(prefix.path() + QLatin1Char('/'));
    };
    for (const auto& path : QCoreApplication::libraryPaths()) {
        if (!insidePrefix(path)) {
            qCritical() << "Plugin search path escaped the payload:" << path;
            return 3;
        }
    }
    if (QFileInfo{QLibraryInfo::path(QLibraryInfo::PluginsPath)}.canonicalFilePath() !=
        prefix.filePath(QStringLiteral("plugins"))) {
        qCritical() << "Unexpected packaged plugin directory";
        return 3;
    }

    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    database.setDatabaseName(QStringLiteral(":memory:"));
    if (!database.open()) {
        qCritical() << database.lastError();
        return 4;
    }
    QSqlQuery query{database};
    if (!query.exec(QStringLiteral("SELECT 42")) || !query.next() || query.value(0).toInt() != 42) {
        qCritical() << query.lastError();
        return 5;
    }

    // Codex Comment: Inspect mappings while the driver is loaded; search paths alone are insufficient.
    QFile maps{QStringLiteral("/proc/self/maps")};
    if (!maps.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot inspect loaded ELF paths";
        return 6;
    }
    QTextStream stream{&maps};
    QSet<QString> checked{};
    bool coreFound{false};
    bool sqlFound{false};
    bool driverFound{false};
    for (auto line = stream.readLine(); !line.isNull(); line = stream.readLine()) {
        const auto start = line.indexOf(QLatin1Char('/'));
        if (start < 0) {
            continue;
        }
        const auto path = line.mid(start);
        const auto name = QFileInfo{path}.fileName();
        if (!name.startsWith(QStringLiteral("libQt6")) &&
            !name.startsWith(QStringLiteral("libicu")) && name != QStringLiteral("libqsqlite.so")) {
            continue;
        }
        if (!insidePrefix(path)) {
            qCritical() << "Loaded library escaped the payload:" << path;
            return 7;
        }
        if (!checked.contains(path)) {
            qInfo().noquote() << "LOADED" << path;
            checked.insert(path);
        }
        coreFound |= name == QStringLiteral("libQt6Core.so.6.10.3");
        sqlFound |= name == QStringLiteral("libQt6Sql.so.6.10.3");
        driverFound |= QFileInfo{path}.canonicalFilePath() ==
            prefix.filePath(QStringLiteral("plugins/sqldrivers/libqsqlite.so"));
    }
    if (!coreFound || !sqlFound || !driverFound) {
        qCritical() << "Required Qt Core, Qt Sql or packaged QSQLITE mapping not observed";
        return 8;
    }
    qInfo() << "PASS: private Qt" << qVersion() << "QSQLITE, in-memory SELECT 42";
    qInfo() << "Plugin paths:" << QCoreApplication::libraryPaths();
    return 0;
}

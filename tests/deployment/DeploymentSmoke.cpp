#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QImageReader>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDebug>

#include <memory>
#include <iostream>
#include <array>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

// Exercise the installed Qt runtime without constructing App or touching JobTracker data.
int main(int argc, char* argv[])
{
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString& message) {
        std::cerr << message.toStdString() << '\n';
    });
    QStandardPaths::setTestModeEnabled(true);
    QGuiApplication application{argc, argv};
    QCoreApplication::setApplicationName(QStringLiteral("JobTrackerDeploymentSmoke"));
    if (QString::fromLatin1(qVersion()) != QStringLiteral("6.10.3") ||
        QGuiApplication::platformName() != QStringLiteral("windows")) {
        qCritical() << "Unexpected Qt version/platform" << qVersion()
                    << QGuiApplication::platformName();
        return 1;
    }

    const QDir prefix{QCoreApplication::applicationDirPath() + QStringLiteral("/..")};
    const auto insidePayload = [&prefix](const QString& path) {
        const auto relative = prefix.relativeFilePath(path);
        return !QDir::isAbsolutePath(relative) && relative != QStringLiteral("..") &&
               !relative.startsWith(QStringLiteral("../"));
    };
    for (const auto& path : QCoreApplication::libraryPaths()) {
        if (!insidePayload(path)) {
            qCritical() << "Plugin lookup escaped payload:" << path;
            return 2;
        }
    }

    QQmlEngine engine{};
    for (const auto& path : engine.importPathList()) {
        if (!path.startsWith(QStringLiteral("qrc:")) && !insidePayload(path)) {
            qCritical() << "QML lookup escaped payload:" << path;
            return 3;
        }
    }
    QQmlComponent component{&engine};
    component.setData(R"qml(
        import QtQuick
        import QtQuick.Controls
        import QtQuick.Dialogs
        import QtQuick.Layouts
        import QtQuick.Shapes
        import QtQuick.Templates as T
        ApplicationWindow {
            visible: false
            width: 320; height: 240
            ColumnLayout {
                Button { text: "Staging check" }
                T.Button { text: "Template check" }
                Shape { ShapePath { startX: 0; startY: 0; PathLine { x: 10; y: 10 } } }
            }
            FileDialog { }
        }
    )qml", QUrl{});
    const std::unique_ptr<QObject> root{component.create()};
    if (!root) {
        qCritical() << component.errors();
        return 4;
    }

    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    database.setDatabaseName(QStringLiteral(":memory:"));
    if (!database.open()) {
        qCritical() << database.lastError();
        return 5;
    }
    QSqlQuery query{database};
    if (!query.exec(QStringLiteral("SELECT 42")) || !query.next() || query.value(0).toInt() != 42) {
        qCritical() << query.lastError();
        return 6;
    }
    const auto formats = QImageReader::supportedImageFormats();
    if (!formats.contains("svg") || !formats.contains("jpeg")) {
        qCritical() << "Missing image plugins:" << formats;
        return 7;
    }
#ifdef Q_OS_WIN
    if (application.arguments().contains(QStringLiteral("--expect-app-local-msvc"))) {
        // A successful run on a development host alone could hide missing CRT
        // files. Verify the loader actually used the private App-Local copies.
        for (const auto* name : {L"msvcp140.dll", L"vcruntime140.dll", L"vcruntime140_1.dll"}) {
            const auto module = GetModuleHandleW(name);
            std::array<wchar_t, 32768> path{};
            const auto length = module ? GetModuleFileNameW(module, path.data(),
                static_cast<DWORD>(path.size())) : 0;
            if (length == 0 || length >= path.size() ||
                QDir::cleanPath(QDir::fromNativeSeparators(QString::fromWCharArray(path.data(),
                    static_cast<int>(length)))).compare(
                        QCoreApplication::applicationDirPath() + QLatin1Char('/') +
                        QString::fromWCharArray(name), Qt::CaseInsensitive) != 0) {
                qCritical() << "MSVC module did not load App-Local:" << QString::fromWCharArray(name);
                return 8;
            }
            qInfo() << "PASS: App-Local MSVC module" << QString::fromWCharArray(path.data());
        }
    }
#endif
    qInfo() << "PASS: Qt" << qVersion() << "windows platform; QML imports; SQLite SELECT; SVG/JPEG";
    qInfo() << "Plugin paths:" << QCoreApplication::libraryPaths();
    qInfo() << "QML paths:" << engine.importPathList();
    return 0;
}

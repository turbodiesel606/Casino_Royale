#include "StorageMutationFeedbackTest.hpp"
#include "maintenance/StorageMutationGate.hpp"
#include "../support/AddJobTestFixture.hpp"
#include "../support/LogCapture.hpp"
#include "app/AppBootstrap.hpp"
#include "logging/ApplicationLog.hpp"
#include "cvs/CvFileAccessService.hpp"
#include "cvs/CvLibraryController.hpp"
#include "dashboard/DashboardController.hpp"
#include "directory/CompanyDirectoryController.hpp"
#include "directory/ContactDirectoryController.hpp"
#include "jobs/JobApplicationsController.hpp"

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QScopeGuard>
#include <QTimer>
#include <QtTest/QtTest>

#include <memory>
#include <mutex>
#include <optional>

void StorageMutationFeedbackTest::repeatedAttemptsReopenDialogAndBannerStaysCurrent()
{
    StorageMutationGate gate;
    QQmlEngine engine;
    QStringList warnings;
    connect(&engine, &QQmlEngine::warnings, &engine, [&](const QList<QQmlError>& errors) {
        for (const auto& error : errors) {
            warnings.append(error.toString());
        }
    });
    engine.rootContext()->setContextProperty(QStringLiteral("gateUnderTest"), &gate);
    const auto feedbackPath = QFINDTESTDATA("../../qml/components/StorageMutationFeedback.qml");
    QVERIFY(!feedbackPath.isEmpty());
    QQmlComponent component{&engine};
    component.setData(R"(
        import QtQuick
        import QtQuick.Controls
        import "." as Components
        ApplicationWindow {
            width: 1180
            height: 720
            visible: true
            property alias feedback: feedback
            Components.StorageMutationFeedback {
                id: feedback
                anchors.left: parent.left
                anchors.right: parent.right
                gate: gateUnderTest
            }
        }
    )", QUrl::fromLocalFile(QFileInfo{feedbackPath}.dir().filePath(QStringLiteral("FeedbackHarness.qml"))));
    QTRY_VERIFY_WITH_TIMEOUT(component.status() != QQmlComponent::Loading, 5000);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    const std::unique_ptr<QObject> window{component.create()};
    QVERIFY2(window != nullptr, qPrintable(component.errorString()));
    auto* const feedback = window->property("feedback").value<QObject*>();
    QVERIFY(feedback != nullptr);
    QCOMPARE(feedback->property("height").toDouble(), 0.0);

    QVERIFY(gate.reserveImportOrUpdate());
    QVERIFY(!gate.checkRemovalAllowed());
    QObject* dialog = nullptr;
    for (auto* child : window->findChildren<QObject*>()) {
        if (child->property("modal").toBool()
            && child->property("title").toString() == gate.blockedTitle()) {
            dialog = child;
            break;
        }
    }
    QVERIFY(dialog != nullptr);
    QTRY_VERIFY(dialog->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(dialog, "accept"));
    QTRY_VERIFY(!dialog->property("visible").toBool());
    QVERIFY(!gate.checkRemovalAllowed());
    QTRY_VERIFY(dialog->property("opened").toBool());
    gate.releaseImportOrUpdate();
    QTRY_VERIFY(feedback->property("height").toDouble() > 0.0);
    QCOMPARE(dialog->property("title").toString(), gate.blockedTitle());
    QVERIFY(dialog->property("title").toString().contains(QStringLiteral("finished")));
    QVERIFY(gate.reserveImportOrUpdate());
    QVERIFY(gate.retryNoticeMessage().contains(QStringLiteral("Wait")));
    QVERIFY(feedback->property("height").toDouble() > 0.0);
    gate.releaseImportOrUpdate();
    gate.dismissRetryNotice();
    QTRY_COMPARE(feedback->property("height").toDouble(), 0.0);
    gate.beginShutdown();
    QTRY_VERIFY(!dialog->property("visible").toBool());
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
}

void StorageMutationFeedbackTest::pendingWorkDialogReportsOnlyExplicitCloseChoices()
{
    testsupport::AddJobWorkerTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto& paths = fixture.storage_.paths();
    StorageMutationGate gate;
    DataRemovalWorker removal{paths.dataDirectory()};
    CvImportWorker imports{paths.dataDirectory(), fixture.cvLock_};
    CvFileAccessService access{paths};
    JobApplicationsController jobs{{}, fixture.worker_, removal, gate};
    CvLibraryController cvs{jobs.jobApplicationListModel(), {}, fixture.cvRepository_, access, imports, removal, gate};
    DashboardController dashboard{jobs.jobApplicationListModel(), cvs.cvListModel()};
    ContactListModel contactModel;
    CompanyDirectoryController companies{jobs.jobApplicationListModel(), contactModel};
    ContactDirectoryController contacts{contactModel};
    QQmlEngine engine;
    auto* context = engine.rootContext();
    context->setContextProperty(QStringLiteral("jobApplicationsController"), &jobs);
    context->setContextProperty(QStringLiteral("cvLibraryController"), &cvs);
    context->setContextProperty(QStringLiteral("dashboardController"), &dashboard);
    context->setContextProperty(QStringLiteral("companyDirectoryController"), &companies);
    context->setContextProperty(QStringLiteral("contactDirectoryController"), &contacts);
    context->setContextProperty(QStringLiteral("storageMutationGate"), &gate);

    const auto mainPath = QFINDTESTDATA("../../qml/Main.qml");
    QVERIFY(!mainPath.isEmpty());
    QQmlComponent component{&engine, QUrl::fromLocalFile(mainPath)};
    QTRY_VERIFY_WITH_TIMEOUT(component.status() != QQmlComponent::Loading, 5000);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root{component.create()};
    auto* window = qobject_cast<QQuickWindow*>(root.get());
    QVERIFY2(window != nullptr, qPrintable(component.errorString()));
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QSignalSpy waits{window, SIGNAL(closeWaitSelected())};
    QSignalSpy interrupts{window, SIGNAL(interruptAndExitSelected())};
    QVERIFY(waits.isValid() && interrupts.isValid());
    QObject* dialog = nullptr;
    QQuickItem* waitButton = nullptr;
    QQuickItem* interruptButton = nullptr;
    for (auto* child : window->findChildren<QObject*>()) {
        if (child->metaObject()->indexOfProperty("opened") >= 0
            && child->property("title").toString() == QStringLiteral("Work in progress"))
            dialog = child;
        if (child->metaObject()->indexOfSignal("clicked()") < 0)
            continue;
        if (child->property("text").toString() == QStringLiteral("Wait"))
            waitButton = qobject_cast<QQuickItem*>(child);
        if (child->property("text").toString() == QStringLiteral("Interrupt and Exit"))
            interruptButton = qobject_cast<QQuickItem*>(child);
    }
    QVERIFY(dialog && waitButton && interruptButton);
    const auto click = [](QQuickItem* item) {
        const auto center = item->mapToScene(item->boundingRect().center()).toPoint();
        QTest::mouseClick(item->window(), Qt::LeftButton, Qt::NoModifier, center);
    };
    const auto source = QUrl::fromLocalFile(fixture.storage_.createFile());

    // Codex Comment: Hold the production CV mutex to keep a real import pending across each close attempt.
    std::unique_lock holdImport{fixture.cvLock_.getMutex()};
    QVERIFY(cvs.addCvs({source}));
    QVERIFY(!window->close());
    QTRY_VERIFY(dialog->property("opened").toBool());
    click(waitButton);
    QTRY_COMPARE(waits.count(), 1);
    QCOMPARE(interrupts.count(), 0);
    QVERIFY(window->isVisible());
    QVERIFY(!gate.shuttingDown());
    QTRY_VERIFY(!dialog->property("visible").toBool());

    QVERIFY(!window->close());
    QTRY_VERIFY(dialog->property("opened").toBool());
    holdImport.unlock();
    QTRY_COMPARE(cvs.pendingImportCount(), 0);
    QTRY_VERIFY(!dialog->property("visible").toBool());
    QCOMPARE(waits.count(), 1);
    QCOMPARE(interrupts.count(), 0);
    QVERIFY(window->isVisible());

    holdImport.lock();
    QVERIFY(cvs.addCvs({source, source, source}));
    QVERIFY(!window->close());
    QTRY_VERIFY(dialog->property("opened").toBool());
    click(interruptButton);
    QTRY_COMPARE(interrupts.count(), 1);
    QCOMPARE(waits.count(), 1);
    QVERIFY(gate.shuttingDown());
    QCOMPARE(gate.shutdownActiveCancellationCount(), 1);
    QCOMPARE(gate.shutdownQueuedCancellationCount(), 2);
    holdImport.unlock();
    QTRY_COMPARE(cvs.pendingImportCount(), 0);
    QTRY_VERIFY(!window->isVisible());
}

void StorageMutationFeedbackTest::bootstrapOpensAndExits_data()
{
    QTest::addColumn<bool>("startLogger");
    QTest::addColumn<bool>("blockOutput");
    QTest::addColumn<bool>("partialCompanies");
    QTest::newRow("disabled") << false << false << false;
    QTest::newRow("writable") << true << false << false;
    QTest::newRow("unwritable") << true << true << false;
    QTest::newRow("partial read") << true << false << true;
    QTest::newRow("partial read without logger") << false << false << true;
}

void StorageMutationFeedbackTest::bootstrapOpensAndExits()
{
    QFETCH(bool, startLogger);
    QFETCH(bool, blockOutput);
    QFETCH(bool, partialCompanies);
    testsupport::LogCapture capture;
    QVERIFY(capture.isValid());
    const auto data = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QCOMPARE(QDir{data}.dirName(), QCoreApplication::applicationName());
    const auto businessData = QDir{data}.filePath(QStringLiteral("Data"));
    QVERIFY(!QFileInfo::exists(businessData));
    const auto cleanup = qScopeGuard([&] { QDir{businessData}.removeRecursively(); });
    QString stagedFile;
    if (partialCompanies) {
        StoragePaths paths;
        SqliteDatabase seed{paths.databasePath()};
        CompanyRepository companies{seed.connection()};
        companies.findOrCreateByName(QStringLiteral("First"));
        companies.findOrCreateByName(QStringLiteral("Second"));
        QSqlQuery query{seed.connection()};
        QVERIFY(query.exec(QStringLiteral("ALTER TABLE companies RENAME TO saved_companies")));
        // Codex Comment: The normalized-name index delivers the first row before SQLite evaluates the second.
        QVERIFY(query.exec(QStringLiteral("CREATE VIEW companies AS SELECT id, normalized_name, created_at, updated_at, "
            "CASE WHEN normalized_name = 'second' THEN abs(-9223372036854775808) ELSE display_name END AS display_name "
            "FROM saved_companies")));
        stagedFile = QDir{paths.resumesDirectory()}.filePath(QStringLiteral("discard.part"));
        QFile staged{stagedFile};
        QVERIFY(staged.open(QIODevice::WriteOnly | QIODevice::NewOnly));
    }
    if (blockOutput) {
        QFile obstruction{capture.directory()};
        QVERIFY(obstruction.open(QIODevice::WriteOnly | QIODevice::NewOnly));
    }
    {
        std::optional<ApplicationLogSession> session;
        if (startLogger)
            session.emplace();
        AppBootstrap bootstrap{*qApp};
        bool closedWindow = false;
        int visibleCompanyCount = -1;
        QTimer::singleShot(0, &bootstrap, [&] {
            for (auto* window : QGuiApplication::topLevelWindows()) {
                if (window->title() == QStringLiteral("JobTracker") && window->isVisible()) {
                    if (auto* context = QQmlEngine::contextForObject(window)) {
                        const auto controller = context->contextProperty(QStringLiteral("companyDirectoryController")).value<QObject*>();
                        if (auto* companies = qobject_cast<CompanyDirectoryController*>(controller))
                            visibleCompanyCount = companies->companyCount();
                    }
                    closedWindow = window->close();
                    break;
                }
            }
            if (!closedWindow)
                QCoreApplication::exit(EXIT_FAILURE);
        });
        QTimer::singleShot(3000, &bootstrap, [] { QCoreApplication::exit(EXIT_FAILURE); });
        QCOMPARE(bootstrap.run(), EXIT_SUCCESS);
        QVERIFY(closedWindow);
        QCOMPARE(visibleCompanyCount, partialCompanies ? 1 : 0);
    }
    QVERIFY(QFileInfo::exists(QDir{businessData}.filePath(QStringLiteral("database.sqlite"))));
    if (partialCompanies)
        QVERIFY(!QFileInfo::exists(stagedFile));
    const auto log = capture.contents();
    if (startLogger && !blockOutput) {
        const auto started = log.indexOf("Startup completed");
        const auto closing = log.indexOf("Controlled shutdown began");
        const auto completed = log.indexOf("Controlled shutdown and required background cleanup completed");
        if (partialCompanies) {
            QCOMPARE(started, -1);
            QVERIFY(log.contains("ERROR | Storage | Message: Could not load companies."));
        } else {
            QVERIFY(started >= 0);
        }
        QVERIFY2(closing > started && completed > closing, log.constData());
    } else {
        QVERIFY(log.isEmpty());
    }
}

QTEST_MAIN(StorageMutationFeedbackTest)

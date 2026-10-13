#include "maintenance/StorageMutationGate.hpp"

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QtTest/QtTest>

#include <memory>

class StorageMutationFeedbackTest final : public QObject
{
    Q_OBJECT

private slots:
    void repeatedAttemptsReopenDialogAndBannerStaysCurrent();
};

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

QTEST_MAIN(StorageMutationFeedbackTest)

#include "StorageMutationFeedbackTest.moc"

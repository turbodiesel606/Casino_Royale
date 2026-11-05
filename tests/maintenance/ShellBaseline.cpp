#include "StorageMutationFeedbackTest.hpp"
#include "ShellBaseline.hpp"

#include "../support/AddJobTestFixture.hpp"
#include "../support/JobApplicationTestData.hpp"
#include "../support/LogCapture.hpp"
#include "app/AppBootstrap.hpp"

#include <QCryptographicHash>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QTimeZone>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest/QtTest>
#include <QtTest/qtestwheel.h>

namespace {

void seedShell(const StoragePaths& paths)
{
    SqliteDatabase database{paths.databasePath()};
    CvRepository cvs{database.connection()};
    const QStringList names{QStringLiteral("Systems.docx"), QStringLiteral("Portfolio.pdf"), QStringLiteral("Legacy.pdf")};
    for (int i = 0; i < names.size(); ++i) {
        CvDocument cv;
        cv.originalFileName_ = names.at(i);
        cv.id_ = cv.originalFileName_;
        cv.storedFileName_ = cv.originalFileName_;
        cv.relativePath_ = QStringLiteral("Resumes/") + cv.storedFileName_;
        cv.title_ = QFileInfo{cv.originalFileName_}.completeBaseName();
        cv.category_ = QStringLiteral("Engineering");
        cv.language_ = QStringLiteral("English");
        cv.createdAt_ = QDateTime{QDate{2026, 5, 12 - i}, QTime{12, 0}, QTimeZone::UTC};
        cv.updatedAt_ = cv.createdAt_;
        if (i == 2)
            cv.applyArchiveState(true, cv.updatedAt_, cv.updatedAt_);
        const auto bytes = cv.title_.toUtf8();
        cv.sha256_ = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        cv.sizeBytes_ = bytes.size();
        QFile file{QDir{paths.resumesDirectory()}.filePath(cv.storedFileName_)};
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::NewOnly));
        QCOMPARE(file.write(bytes), bytes.size());
        cvs.insert(cv);
    }
    CompanyRepository companies{database.connection()};
    const auto company = companies.findOrCreateByName(QStringLiteral("Example Company"));
    JobRepository jobs{database.connection()};
    jobs.insert(testsupport::makeJobApplication(QStringLiteral("job-shell"), company.id_, company.name_,
        QStringLiteral("Systems Engineer"), names.first(), names.first(), QDate{2026, 5, 12},
        JobStatus::Applied, QStringLiteral("Interview")));
}

QQuickItem* findByProperty(QQuickWindow& window, const char* name, const QString& value)
{
    return visibleItem(window.contentItem(), [&](QQuickItem* item) { return item->property(name).toString() == value; });
}

bool click(QQuickItem* item)
{
    if (!item || !item->isVisible() || !item->isEnabled())
        return false;
    QTest::mouseClick(item->window(), Qt::LeftButton, Qt::NoModifier,
        item->mapToScene(item->boundingRect().center()).toPoint());
    return true;
}

QQuickItem* buttonWithLabel(QQuickWindow& window, const QString& label)
{
    return visibleItem(window.contentItem(), [&](QQuickItem* item) {
        return item->metaObject()->indexOfSignal("clicked()") >= 0
            && (item->property("text").toString() == label || item->property("modelData").toString() == label);
    });
}

bool pressButton(QQuickWindow& window, const QString& label)
{
    return click(buttonWithLabel(window, label));
}

bool waitForNextFrame(QQuickWindow& window)
{
    QSignalSpy frames{&window, &QQuickWindow::frameSwapped};
    window.update();
    return QTest::qWaitFor([&frames] { return !frames.isEmpty(); });
}

QObject* dialogTitled(QQuickWindow& window, const QString& title)
{
    for (auto* object : window.findChildren<QObject*>()) {
        if (object->property("title").toString() == title && object->metaObject()->indexOfProperty("opened") >= 0)
            return object;
    }
    return nullptr;
}

void capture(QQuickWindow& window, const QString& name)
{
    const auto directory = qEnvironmentVariable("JOBTRACKER_BASELINE_IMAGE_DIR");
    if (directory.isEmpty())
        return;
    const auto frame = window.grabWindow();
    QVERIFY(!frame.isNull());
    const auto fileName = QStringLiteral("%1x%2-%3.png").arg(window.width()).arg(window.height()).arg(name);
    QFile output{QDir{directory}.filePath(fileName)};
    QVERIFY2(output.open(QIODevice::WriteOnly | QIODevice::NewOnly), qPrintable(output.errorString()));
    QVERIFY2(frame.save(&output, "PNG"), qPrintable(fileName));
}

QQuickItem* viewportFor(QQuickWindow& window, const QString& label)
{
    QQuickItem* viewport = nullptr;
    visibleItem(window.contentItem(), [&](QQuickItem* item) {
        if (item->property("text").toString() != label)
            return false;
        for (auto* ancestor = item->parentItem(); ancestor; ancestor = ancestor->parentItem()) {
            if (ancestor->metaObject()->indexOfProperty("contentY") >= 0) {
                viewport = ancestor;
                return true;
            }
        }
        return false;
    });
    return viewport;
}

void checkScrolling(QQuickWindow& window, const QString& page, const QString& label)
{
    window.grabWindow();
    auto* viewport = viewportFor(window, label);
    QVERIFY2(viewport, qPrintable(label));
    QTRY_VERIFY(viewport->height() > 0 && viewport->property("contentHeight").toReal() > 0);
    const qreal content = viewport->property("contentHeight").toReal();
    const qreal maximum = qMax(0.0, content - viewport->height());
    const auto offset = [&] { return viewport->property("contentY").toReal(); };
    auto* track = visibleItem(window.contentItem(), [](QQuickItem* item) {
        return item->property("color").value<QColor>() == QColor{QStringLiteral("#0a1823")}
            && item->height() > 100 && item->width() < 20;
    });
    qInfo().nospace() << "Scroll " << page << " viewport=" << viewport->height() << " content=" << content
                      << " track=" << (track != nullptr) << " viewportScene=" << viewport->mapRectToScene(viewport->boundingRect());
    QCOMPARE(track != nullptr, maximum > 0);
    QVERIFY(qAbs(offset()) <= 1);
    capture(window, page);
    auto* heading = visibleItem(viewport, [&](QQuickItem* item) { return item->property("text").toString() == label; });
    QVERIFY(heading);
    const auto wheelAt = heading->mapToScene(QPointF{2, heading->height() / 2});
    QVERIFY(window.contentItem()->boundingRect().contains(wheelAt));
    QTest::mouseMove(&window, wheelAt.toPoint());
    QSignalSpy wheelChanges{viewport, SIGNAL(contentYChanged())};
    QTest::wheelEvent(&window, wheelAt, {0, -120});
    if (!track) {
        QCOMPARE(offset(), 0.0);
        return;
    }
    const bool detentMoved = QTest::qWaitFor([&] { return offset() > 0; }, 1000);
    qInfo() << page << "wheel detent moved" << detentMoved << "offset" << offset() << "changes" << wheelChanges.count();
    // Codex Comment: Wheel delivery to the focused editor differed on R0 Windows offscreen. Keep its observation for native comparison.
    if (page != QStringLiteral("job-description"))
        QVERIFY(detentMoved);
    QTRY_VERIFY_WITH_TIMEOUT(!viewport->property("moving").toBool(), 1500);
    const auto onTrack = [track](qreal y) { return track->mapToScene(QPointF{track->width() / 2, y}).toPoint(); };
    qInfo() << "Track scene" << track->mapRectToScene(track->boundingRect()) << "wheel" << wheelAt;
    QVERIFY(window.contentItem()->boundingRect().contains(onTrack(1)));
    QVERIFY(window.contentItem()->boundingRect().contains(onTrack(track->height() - 1)));
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, onTrack(1));
    QTRY_VERIFY(qAbs(offset()) <= 1);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, onTrack(track->height() - 1));
    QTRY_VERIFY(qAbs(offset() - maximum) <= 1);
    auto* thumb = visibleItem(window.contentItem(), [](QQuickItem* item) {
        return item->property("color").value<QColor>() == QColor{QStringLiteral("#a8b0b6")}
            && item->width() < 20 && item->height() >= 42;
    });
    QVERIFY(thumb);
    QCOMPARE(thumb->width(), track->width() - 3);
    const qreal expectedHeight = qBound(42.0, track->height() * viewport->height() / content, track->height());
    QVERIFY(qAbs(thumb->height() - expectedHeight) <= 1);
    const qreal travel = track->height() - thumb->height();
    QVERIFY(travel > 0);
    const auto midpoint = track->mapToScene(QPointF{1, track->height() / 2}).toPoint();
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, midpoint);
    const qreal tolerance = 1 + maximum / travel;
    QTRY_VERIFY(qAbs(offset() - maximum / 2) <= tolerance);
    const auto dragTo = [&](const QPoint& end) {
        const auto start = thumb->mapToScene(thumb->boundingRect().center()).toPoint();
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(&window, end);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, end);
    };
    dragTo(onTrack(track->height() / 2 + travel / 4));
    QTRY_VERIFY(qAbs(offset() - 3 * maximum / 4) <= 2 * tolerance);
    dragTo(onTrack(1));
    QTRY_VERIFY(qAbs(offset()) <= 1);
    dragTo(onTrack(track->height() - 1));
    QTRY_VERIFY(qAbs(offset() - maximum) <= 1);
    capture(window, page + QStringLiteral("-bottom"));
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, onTrack(1));
    QTRY_VERIFY(qAbs(offset()) <= 1);
}

int visiblePdfLabels(QQuickItem* item)
{
    if (!item->isVisible())
        return 0;
    int labels = item->property("text").toString() == QStringLiteral("PDF") ? 1 : 0;
    for (auto* child : item->childItems())
        labels += visiblePdfLabels(child);
    return labels;
}

void exerciseCvPage(QQuickWindow& window, const StoragePaths& paths)
{
    auto* context = qmlContext(&window);
    QVERIFY(context);
    auto* cvs = context->contextProperty(QStringLiteral("cvLibraryController")).value<CvLibraryController*>();
    auto* jobs = context->contextProperty(QStringLiteral("jobApplicationsController")).value<JobApplicationsController*>();
    QVERIFY(cvs && jobs);
    QVERIFY(pressButton(window, QStringLiteral("CV Library")));
    QTRY_COMPARE(window.property("currentPage").toInt(), 2);
    QVERIFY(waitForNextFrame(window));
    QTRY_COMPARE(cvs->selectedCvId(), QStringLiteral("Systems.docx"));
    QTRY_COMPARE(visiblePdfLabels(window.contentItem()), 3);
    capture(window, QStringLiteral("cv-active"));
    QVERIFY(click(findByProperty(window, "text", QStringLiteral("Portfolio.pdf"))));
    QTRY_COMPARE(cvs->selectedCvId(), QStringLiteral("Portfolio.pdf"));
    QVERIFY(pressButton(window, QStringLiteral("Archived")));
    QTRY_COMPARE(cvs->selectedCvId(), QStringLiteral("Legacy.pdf"));
    QTRY_COMPARE(visiblePdfLabels(window.contentItem()), 2);
    capture(window, QStringLiteral("cv-archived"));
    QVERIFY(pressButton(window, QStringLiteral("Active")));
    auto* search = findByProperty(window, "placeholderText", QStringLiteral("Search CVs..."));
    QVERIFY(click(search));
    QTest::keyClick(&window, Qt::Key_Z);
    QTRY_COMPARE(cvs->cvCount(), 0);
    QTRY_COMPARE(visiblePdfLabels(window.contentItem()), 1);
    capture(window, QStringLiteral("cv-empty"));
    QTest::keyClick(&window, Qt::Key_Backspace);
    QTRY_COMPARE(cvs->cvCount(), 2);

    cvs->toggleCvChecked(0);
    QCOMPARE(cvs->checkedCvIds(), QStringList{QStringLiteral("Systems.docx")});
    auto* confirmation = dialogTitled(window, QStringLiteral("Remove CVs from the library"));
    QVERIFY(confirmation);
    const auto previous = confirmation->property("message").toString();
    QVERIFY(previous.contains(QStringLiteral("1 linked CV(s)")) && previous.contains(QStringLiteral("0 unlinked CV(s)")));
    QSignalSpy changed{cvs, &CvLibraryController::checkedCvsChanged};
    QSignalSpy completed{jobs, &JobApplicationsController::applicationUpdateCompleted};
    jobs->updateApplication(QStringLiteral("job-shell"), testsupport::validJobFormValues(QStringLiteral("Systems Engineer")),
        QUrl::fromLocalFile(QDir{paths.resumesDirectory()}.filePath(QStringLiteral("Portfolio.pdf"))));
    QTRY_COMPARE(completed.count(), 1);
    QVERIFY(completed.first().at(3).toBool());
    QCOMPARE(cvs->checkedLinkedCvCount(), 0);
    QCOMPARE(cvs->checkedUnlinkedCvCount(), 1);
    // Codex Comment: F002 at R0 leaves the already bound confirmation counts stale after the last link moves.
    QCOMPARE(changed.count(), 0);
    QVERIFY(pressButton(window, QStringLiteral("Remove Selected (1)")));
    QTRY_VERIFY(confirmation->property("opened").toBool());
    QCOMPARE(confirmation->property("message").toString(), previous);
    capture(window, QStringLiteral("cv-confirmation"));
    QVERIFY(QMetaObject::invokeMethod(confirmation, "reject"));
}

void exerciseShell(QQuickWindow& window, const QSize& size, const StoragePaths& paths)
{
    window.resize(size);
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_COMPARE(window.size(), size);
    qInfo() << "Shell size" << window.size() << "devicePixelRatio" << window.devicePixelRatio()
            << "platform" << QGuiApplication::platformName();
    checkScrolling(window, QStringLiteral("dashboard"), QStringLiteral("Dashboard"));
    if (QTest::currentTestFailed())
        return;

    QVERIFY(pressButton(window, QStringLiteral("+   Add Job")));
    QTRY_VERIFY(window.property("jobFormVisible").toBool());
    checkScrolling(window, QStringLiteral("add-job"), QStringLiteral("Job Title"));
    if (QTest::currentTestFailed())
        return;
    QVERIFY(pressButton(window, QStringLiteral("Save")));
    QTRY_VERIFY(findByProperty(window, "text", QStringLiteral("Please correct the highlighted fields.")));
    capture(window, QStringLiteral("add-validation"));

    QVERIFY(pressButton(window, QStringLiteral("Job Applications")));
    QTRY_COMPARE(window.property("currentPage").toInt(), 1);
    // Codex Comment: Wait for each newly shown StackLayout page to render before sending pointer input.
    QVERIFY(waitForNextFrame(window));
    auto* descriptionTab = buttonWithLabel(window, QStringLiteral("Job Description"));
    QVERIFY(click(descriptionTab));
    QTRY_VERIFY(descriptionTab->property("selected").toBool());
    QVERIFY(waitForNextFrame(window));
    QVERIFY(pressButton(window, QStringLiteral("Edit")));
    QTRY_VERIFY(window.activeFocusItem()
        && window.activeFocusItem()->property("text").toString() == QStringLiteral("Systems Engineer"));
    auto* title = window.activeFocusItem();
    QTest::keyClick(&window, Qt::Key_End);
    QTest::keyClick(&window, Qt::Key_Z);
    const auto draft = title->property("text").toString();
    QVERIFY(draft.endsWith(QLatin1Char('z'), Qt::CaseInsensitive));
    checkScrolling(window, QStringLiteral("job-description"), QStringLiteral("Job Title"));
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(title->property("text").toString(), draft);
    QCOMPARE(window.activeFocusItem(), title);
    QVERIFY(pressButton(window, QStringLiteral("Companies")));
    auto* unsaved = dialogTitled(window, QStringLiteral("Unsaved changes"));
    QVERIFY(unsaved);
    QTRY_VERIFY(unsaved->property("opened").toBool());
    QVERIFY(pressButton(window, QStringLiteral("Cancel")));
    QCOMPARE(window.property("currentPage").toInt(), 1);
    QCOMPARE(title->property("text").toString(), draft);
    QVERIFY(pressButton(window, QStringLiteral("Companies")));
    QTRY_VERIFY(unsaved->property("opened").toBool());
    if (size.height() == 1800) {
        auto* jobs = qmlContext(&window)->contextProperty(QStringLiteral("jobApplicationsController")).value<JobApplicationsController*>();
        QVERIFY(jobs);
        QSignalSpy completed{jobs, &JobApplicationsController::applicationUpdateCompleted};
        bool sawSaving = false;
        QObject observer;
        QObject::connect(jobs, &JobApplicationsController::applicationUpdateQueued, &observer, [&] {
            sawSaving = window.property("currentPage").toInt() == 1
                && findByProperty(window, "text", QStringLiteral("Saving Job Description..."));
            capture(window, QStringLiteral("editor-saving"));
        });
        QVERIFY(pressButton(window, QStringLiteral("Save")));
        QTRY_COMPARE(window.property("currentPage").toInt(), 3);
        QVERIFY(sawSaving);
        QCOMPARE(completed.count(), 1);
        QVERIFY(completed.first().at(3).toBool());
        QCOMPARE(jobs->selectedApplication().value(QStringLiteral("jobTitle")).toString(), draft);
        QVERIFY(pressButton(window, QStringLiteral("Dashboard")));
        QTRY_COMPARE(window.property("currentPage").toInt(), 0);
        QTRY_VERIFY(findByProperty(window, "text", draft));
        SqliteDatabase reader{paths.databasePath()};
        JobRepository storedJobs{reader.connection()};
        const auto stored = storedJobs.findById(QStringLiteral("job-shell"));
        QVERIFY(stored.has_value());
        QCOMPARE(stored->jobTitle_, draft);
        return;
    }
    QVERIFY(pressButton(window, QStringLiteral("Don't Save")));
    QTRY_COMPARE(window.property("currentPage").toInt(), 3);
    QVERIFY(pressButton(window, QStringLiteral("Contacts")));
    QTRY_COMPARE(window.property("currentPage").toInt(), 4);
    exerciseCvPage(window, paths);
}

} // namespace

void StorageMutationFeedbackTest::shellInteractions_data()
{
    QTest::addColumn<QSize>("size");
    QTest::newRow("default") << QSize{1660, 948};
    QTest::newRow("minimum") << QSize{1180, 720};
    QTest::newRow("tall") << QSize{1660, 1800};
}

void StorageMutationFeedbackTest::shellInteractions()
{
    QFETCH(QSize, size);
    testsupport::LogCapture profile;
    QVERIFY(profile.isValid());
    const auto appData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QCOMPARE(QDir{appData}.dirName(), QCoreApplication::applicationName());
    const auto business = QDir{appData}.filePath(QStringLiteral("Data"));
    QVERIFY(!QFileInfo::exists(business));
    const auto cleanup = qScopeGuard([&] { QDir{business}.removeRecursively(); });
    StoragePaths paths{business};
    seedShell(paths);
    if (QTest::currentTestFailed())
        return;
    AppBootstrap bootstrap{*qApp};
    bool exercised = false;
    QTimer::singleShot(0, &bootstrap, [&] {
        for (auto* candidate : QGuiApplication::topLevelWindows()) {
            auto* window = qobject_cast<QQuickWindow*>(candidate);
            if (window && window->isVisible() && window->title() == QStringLiteral("JobTracker")) {
                exerciseShell(*window, size, paths);
                exercised = true;
                break;
            }
        }
        QCoreApplication::exit();
    });
    QCOMPARE(bootstrap.run(), EXIT_SUCCESS);
    QVERIFY(exercised);
}

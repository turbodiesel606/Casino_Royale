#include "cvs/CvFileAccessService.hpp"
#include "cvs/CvLibraryController.hpp"
#include "cvs/CvRepository.hpp"
#include "jobs/JobApplicationListModel.hpp"
#include "storage/SqliteDatabase.hpp"
#include "storage/StoragePaths.hpp"

#include "../support/JobApplicationTestData.hpp"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

int roleForName(const QAbstractItemModel& model, const QByteArray& roleName)
{
    const auto names = model.roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        if (it.value() == roleName) {
            return it.key();
        }
    }
    return -1;
}

class CvTestStorage final
{
public:
    CvTestStorage()
        : paths_(QDir(temporaryDirectory_.path()).filePath(QStringLiteral("Data")))
        , database_(paths_.databasePath())
        , repository_(database_.connection())
        , fileAccessService_(paths_)
    {
    }

    QTemporaryDir temporaryDirectory_;
    StoragePaths paths_;
    SQLiteDataBase database_;
    CvRepository repository_;
    CvFileAccessService fileAccessService_;
};

QVector<CvDocument> makeCvDocuments()
{
    QVector<CvDocument> documents;
    for (int index = 0; index < 4; ++index) {
        CvDocument document;
        document.id_ = QStringList{
            QStringLiteral("cv-qt-2026"),
            QStringLiteral("cv-embedded"),
            QStringLiteral("cv-general"),
            QStringLiteral("cv-backend")}.at(index);
        document.fileName_ = QStringList{
            QStringLiteral("CV_Qt_2026.pdf"),
            QStringLiteral("CV_Embedded.pdf"),
            QStringLiteral("CV_General.pdf"),
            QStringLiteral("CV_Backend.pdf")}.at(index);
        document.originalFileName_ = document.fileName_;
        document.storedFileName_ = QStringLiteral("stored-%1.pdf").arg(index);
        document.relativePath_ = QStringLiteral("Resumes/%1").arg(document.storedFileName_);
        document.sha256_ = QStringLiteral("test-hash-%1").arg(index);
        document.sizeBytes_ = 1024 + index;
        document.title_ = document.fileName_;
        document.category_ = index == 2 ? QStringLiteral("General") : QStringLiteral("Engineering");
        document.language_ = QStringLiteral("English");
        document.lastModifiedLabel_ = QStringLiteral("May %1, 2026").arg(12 - index);
        document.isFavorite_ = index == 0;
        document.createdAt_ = QStringLiteral("2026-05-%1T10:00:00Z").arg(12 - index, 2, 10, QLatin1Char('0'));
        document.updatedAt_ = document.createdAt_;
        documents.append(document);
    }
    documents[0].linkedApplicationIds_ = {
        QStringLiteral("job-kdab-cpp-qt"),
        QStringLiteral("job-techsoft-qt-qml"),
        QStringLiteral("job-codecraft-cpp-qt"),
        QStringLiteral("job-innotech-qt-qml")};
    documents[1].linkedApplicationIds_ = {QStringLiteral("job-vision-embedded")};
    documents[2].linkedApplicationIds_ = {
        QStringLiteral("job-greenwidget-software"),
        QStringLiteral("job-byteworks-software"),
        QStringLiteral("job-platforma-cpp")};
    return documents;
}

}

class CvLibraryControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void cvModelExposesNamedRoles();
    void cvModelExposesSeedDocuments();
    void selectedCvControlsLinkedApplications();
    void favoriteToggleUpdatesSelectedCv();
    void favoriteFailureLeavesModelUnchanged();
    void fileAccessRejectsInvalidManagedPaths();
    void fileAccessRejectsTraversalPaths();
    void fileAccessRejectsMissingFiles();
    void openCvPublishesFileAccessFailure();
    void controllerFiltersAndSortsCvs();
    void selectionRemainsStableAcrossProxyChanges();
};

void CvLibraryControllerTest::cvModelExposesNamedRoles()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel;
    CvLibraryController controller(
        applicationsModel,
        makeCvDocuments(),
        storage.repository_,
        storage.fileAccessService_);
    const auto* model = controller.cvModel();

    QVERIFY(roleForName(*model, "id") > 0);
    QVERIFY(roleForName(*model, "fileName") > 0);
    QVERIFY(roleForName(*model, "categoryAccent") > 0);
    QVERIFY(roleForName(*model, "languageAccent") > 0);
    QVERIFY(roleForName(*model, "linkedApplicationCount") > 0);
    QVERIFY(roleForName(*model, "linkedApplicationCountLabel") > 0);
    QVERIFY(roleForName(*model, "isFavorite") > 0);
}

void CvLibraryControllerTest::cvModelExposesSeedDocuments()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel;
    CvLibraryController controller(
        applicationsModel,
        makeCvDocuments(),
        storage.repository_,
        storage.fileAccessService_);
    const auto* model = controller.cvModel();
    const auto firstRow = model->index(0, 0);

    QCOMPARE(model->rowCount(), 4);
    QCOMPARE(model->data(firstRow, roleForName(*model, "id")).toString(), QStringLiteral("cv-qt-2026"));
    QCOMPARE(model->data(firstRow, roleForName(*model, "fileName")).toString(), QStringLiteral("CV_Qt_2026.pdf"));
    QCOMPARE(model->data(firstRow, roleForName(*model, "linkedApplicationCount")).toInt(), 4);
    QCOMPARE(controller.resultSummary(), QStringLiteral("4 CVs"));
}

void CvLibraryControllerTest::selectedCvControlsLinkedApplications()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel(testsupport::makeJobApplications());
    CvLibraryController controller(
        applicationsModel,
        makeCvDocuments(),
        storage.repository_,
        storage.fileAccessService_);
    QSignalSpy selectedIndexSpy(&controller, &CvLibraryController::selectedCvIndexChanged);
    QSignalSpy selectedIdSpy(&controller, &CvLibraryController::selectedCvIdChanged);
    QSignalSpy selectedDataSpy(&controller, &CvLibraryController::selectedCvChanged);
    QSignalSpy linkedResetSpy(controller.linkedApplicationsModel(), &QAbstractItemModel::modelReset);

    controller.selectCv(2);

    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedIdSpy.count(), 1);
    QCOMPARE(selectedDataSpy.count(), 1);
    QCOMPARE(linkedResetSpy.count(), 1);
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-general"));
    QCOMPARE(controller.selectedCv().value(QStringLiteral("fileName")).toString(), QStringLiteral("CV_General.pdf"));

    const auto* linkedModel = controller.linkedApplicationsModel();
    QCOMPARE(linkedModel->rowCount(), 3);
    const auto firstLinkedRow = linkedModel->index(0, 0);
    QCOMPARE(linkedModel->data(firstLinkedRow, roleForName(*linkedModel, "id")).toString(), QStringLiteral("job-greenwidget-software"));
    QCOMPARE(linkedModel->data(firstLinkedRow, roleForName(*linkedModel, "cvFileName")).toString(), QStringLiteral("CV_General.pdf"));
}

void CvLibraryControllerTest::favoriteToggleUpdatesSelectedCv()
{
    CvTestStorage storage;
    const auto documents = makeCvDocuments();
    storage.repository_.insert(documents.first());
    JobApplicationListModel applicationsModel;
    CvLibraryController controller(
        applicationsModel,
        documents,
        storage.repository_,
        storage.fileAccessService_);
    QSignalSpy selectedIndexSpy(&controller, &CvLibraryController::selectedCvIndexChanged);
    QSignalSpy selectedIdSpy(&controller, &CvLibraryController::selectedCvIdChanged);
    QSignalSpy selectedDataSpy(&controller, &CvLibraryController::selectedCvChanged);
    QSignalSpy failedSpy(&controller, &CvLibraryController::operationFailed);

    QVERIFY(controller.selectedCv().value(QStringLiteral("isFavorite")).toBool());
    controller.toggleFavorite(QStringLiteral("cv-qt-2026"));

    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(selectedIndexSpy.count(), 0);
    QCOMPARE(selectedIdSpy.count(), 0);
    QCOMPARE(selectedDataSpy.count(), 1);
    QVERIFY(!controller.selectedCv().value(QStringLiteral("isFavorite")).toBool());
    QVERIFY(!storage.repository_.findAll().first().isFavorite_);
}

void CvLibraryControllerTest::favoriteFailureLeavesModelUnchanged()
{
    CvTestStorage storage;
    auto documents = makeCvDocuments();
    documents.first().isFavorite_ = false;
    storage.repository_.insert(documents.first());

    QSqlQuery trigger{storage.database_.connection()};
    QVERIFY(trigger.exec(QStringLiteral(
        "CREATE TRIGGER reject_favorite_update BEFORE UPDATE OF is_favorite ON cvs "
        "BEGIN SELECT RAISE(FAIL, 'forced favorite failure'); END")));

    JobApplicationListModel applicationsModel;
    CvLibraryController controller(
        applicationsModel,
        documents,
        storage.repository_,
        storage.fileAccessService_);
    QSignalSpy failedSpy(&controller, &CvLibraryController::operationFailed);
    QSignalSpy dataChangedSpy(
        &controller.cvListModel(),
        &QAbstractItemModel::dataChanged);

    controller.toggleFavorite(QStringLiteral("cv-qt-2026"));

    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(dataChangedSpy.count(), 0);
    QVERIFY(!controller.selectedCv().value(QStringLiteral("isFavorite")).toBool());
    QVERIFY(!storage.repository_.findAll().first().isFavorite_);
}

void CvLibraryControllerTest::fileAccessRejectsInvalidManagedPaths()
{
    CvTestStorage storage;
    CvDocument document;

    auto result = storage.fileAccessService_.openDocument(document);
    QVERIFY(!result.opened_);
    QVERIFY(!result.message_.isEmpty());

    const auto absolutePath = QDir(storage.paths_.resumesDirectory()).filePath(QStringLiteral("absolute.pdf"));
    QFile file{absolutePath};
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArrayLiteral("%PDF-1.4 test"));
    file.close();

    document.relativePath_ = absolutePath;
    result = storage.fileAccessService_.openDocument(document);
    QVERIFY(!result.opened_);
    QVERIFY(!result.message_.isEmpty());
}

void CvLibraryControllerTest::fileAccessRejectsTraversalPaths()
{
    CvTestStorage storage;
    CvDocument document;
    document.relativePath_ = QStringLiteral("Resumes/../outside.pdf");

    const auto result = storage.fileAccessService_.openDocument(document);

    QVERIFY(!result.opened_);
    QVERIFY(!result.message_.isEmpty());
}

void CvLibraryControllerTest::fileAccessRejectsMissingFiles()
{
    CvTestStorage storage;
    CvDocument document;
    document.relativePath_ = QStringLiteral("Resumes/missing.pdf");

    const auto result = storage.fileAccessService_.openDocument(document);

    QVERIFY(!result.opened_);
    QVERIFY(!result.message_.isEmpty());
}

void CvLibraryControllerTest::openCvPublishesFileAccessFailure()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel;
    CvLibraryController controller(
        applicationsModel,
        makeCvDocuments(),
        storage.repository_,
        storage.fileAccessService_);
    QSignalSpy failedSpy(&controller, &CvLibraryController::operationFailed);

    controller.openCv(QStringLiteral("cv-qt-2026"));

    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(!failedSpy.first().first().toString().isEmpty());
}

void CvLibraryControllerTest::controllerFiltersAndSortsCvs()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel(testsupport::makeJobApplications());
    CvLibraryController controller(
        applicationsModel,
        makeCvDocuments(),
        storage.repository_,
        storage.fileAccessService_);
    QSignalSpy categorySummarySpy(&controller, &CvLibraryController::categorySummaryChanged);
    QSignalSpy categoryFilterSpy(&controller, &CvLibraryController::categoryFilterChanged);
    QSignalSpy countSpy(&controller, &CvLibraryController::cvCountChanged);
    QSignalSpy resultSummarySpy(&controller, &CvLibraryController::resultSummaryChanged);

    controller.setSearchText(QStringLiteral("embedded"));

    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(resultSummarySpy.count(), 1);
    QCOMPARE(controller.cvCount(), 1);
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-embedded"));
    QCOMPARE(controller.resultSummary(), QStringLiteral("1 CV"));

    controller.clearFilters();
    controller.setCategoryFilter(QStringLiteral("General"));

    QCOMPARE(categoryFilterSpy.count(), 1);
    QCOMPARE(categorySummarySpy.count(), 1);
    QCOMPARE(countSpy.count(), 3);
    QCOMPARE(resultSummarySpy.count(), 3);
    QCOMPARE(controller.cvCount(), 1);
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-general"));

    controller.clearFilters();
    controller.setSortMode(QStringLiteral("Linked Jobs"));

    QCOMPARE(categoryFilterSpy.count(), 2);
    QCOMPARE(categorySummarySpy.count(), 2);
    QCOMPARE(controller.cvCount(), 4);
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-general"));
    QCOMPARE(controller.cvModel()->data(controller.cvModel()->index(0, 0), roleForName(*controller.cvModel(), "linkedApplicationCount")).toInt(), 4);
}

void CvLibraryControllerTest::selectionRemainsStableAcrossProxyChanges()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel{testsupport::makeJobApplications()};
    CvLibraryController controller{
        applicationsModel,
        makeCvDocuments(),
        storage.repository_,
        storage.fileAccessService_};
    QSignalSpy selectedIndexSpy{&controller, &CvLibraryController::selectedCvIndexChanged};
    QSignalSpy selectedIdSpy{&controller, &CvLibraryController::selectedCvIdChanged};
    QSignalSpy selectedDataSpy{&controller, &CvLibraryController::selectedCvChanged};
    QSignalSpy linkedResetSpy{controller.linkedApplicationsModel(), &QAbstractItemModel::modelReset};

    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-qt-2026"));
    controller.setSortMode(QStringLiteral("File Name"));
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-qt-2026"));
    QCOMPARE(controller.selectedCvIndex(), 3);
    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedIdSpy.count(), 0);
    QCOMPARE(selectedDataSpy.count(), 0);
    QCOMPARE(linkedResetSpy.count(), 0);

    controller.setCategoryFilter(QStringLiteral("Engineering"));
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-qt-2026"));
    QCOMPARE(controller.selectedCvIndex(), 2);
    QCOMPARE(linkedResetSpy.count(), 0);

    controller.clearFilters();
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-qt-2026"));
    QCOMPARE(controller.selectedCvIndex(), 3);
    QCOMPARE(linkedResetSpy.count(), 0);

    controller.setCategoryFilter(QStringLiteral("General"));
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-general"));
    QCOMPARE(controller.selectedCvIndex(), 0);
    QCOMPARE(linkedResetSpy.count(), 1);

    controller.clearFilters();
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-general"));
    QCOMPARE(controller.selectedCvIndex(), 2);
    QCOMPARE(linkedResetSpy.count(), 1);

    controller.setSearchText(QStringLiteral("does-not-match"));
    QCOMPARE(controller.cvCount(), 0);
    QCOMPARE(controller.selectedCvIndex(), -1);
    QVERIFY(controller.selectedCvId().isEmpty());
    QCOMPARE(linkedResetSpy.count(), 2);

    controller.clearFilters();
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-backend"));
    QCOMPARE(controller.selectedCvIndex(), 0);
    QCOMPARE(linkedResetSpy.count(), 3);

    selectedIndexSpy.clear();
    selectedIdSpy.clear();
    selectedDataSpy.clear();
    linkedResetSpy.clear();
    QSignalSpy categorySummarySpy{&controller, &CvLibraryController::categorySummaryChanged};
    QSignalSpy countSpy{&controller, &CvLibraryController::cvCountChanged};
    QSignalSpy resultSummarySpy{&controller, &CvLibraryController::resultSummaryChanged};
    auto insertedDocument = makeCvDocuments().first();
    insertedDocument.id_ = QStringLiteral("cv-alphabetical-first");
    insertedDocument.fileName_ = QStringLiteral("AAA_CV.pdf");
    insertedDocument.originalFileName_ = insertedDocument.fileName_;
    insertedDocument.storedFileName_ = QStringLiteral("stored-alphabetical-first.pdf");
    insertedDocument.relativePath_ = QStringLiteral("Resumes/stored-alphabetical-first.pdf");
    insertedDocument.sha256_ = QStringLiteral("test-hash-alphabetical-first");
    insertedDocument.linkedApplicationIds_.clear();

    controller.recordCvUse(
        insertedDocument,
        QStringLiteral("job-alphabetical-first"),
        true);

    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-backend"));
    QCOMPARE(controller.selectedCvIndex(), 1);
    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedIdSpy.count(), 0);
    QCOMPARE(selectedDataSpy.count(), 0);
    QCOMPARE(linkedResetSpy.count(), 0);
    QCOMPARE(categorySummarySpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(resultSummarySpy.count(), 1);
}

QTEST_GUILESS_MAIN(CvLibraryControllerTest)

#include "CvLibraryControllerTest.moc"

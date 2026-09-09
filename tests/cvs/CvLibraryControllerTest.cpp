#include "cvs/CvFileAccessService.hpp"
#include "cvs/CvImportService.hpp"
#include "cvs/CvImportWorker.hpp"
#include "cvs/CvLibraryController.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvMutationQueue.hpp"
#include "cvs/CvRepository.hpp"
#include "jobs/JobApplicationListModel.hpp"
#include "jobs/JobRepository.hpp"
#include "maintenance/DataRemovalWorker.hpp"
#include "maintenance/StorageMutationGate.hpp"
#include "storage/SqliteDatabase.hpp"
#include "storage/SqlTransaction.hpp"
#include "storage/StoragePaths.hpp"

#include "../support/JobApplicationTestData.hpp"
#include "../support/AddJobTestFixture.hpp"
#include "jobs/JobApplicationsController.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

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
        , importWorker_{paths_.dataDirectory(), cvMutationQueue_}
    {
    }

    QString createSourceFile(
        const QString& fileName,
        const QByteArray& contents = QByteArrayLiteral("%PDF-1.4 JobTracker CV"),
        const QString& subdirectory = {}) const
    {
        auto sourceDirectory = QDir{temporaryDirectory_.path()}
            .filePath(QStringLiteral("Sources"));
        if (!subdirectory.isEmpty()) {
            sourceDirectory = QDir{sourceDirectory}.filePath(subdirectory);
        }
        if (!QDir{}.mkpath(sourceDirectory)) {
            return {};
        }

        const auto sourcePath = QDir{sourceDirectory}.filePath(fileName);
        QFile source{sourcePath};
        if (!source.open(QIODevice::WriteOnly | QIODevice::Truncate)
            || source.write(contents) != contents.size()) {
            return {};
        }
        return sourcePath;
    }

    QTemporaryDir temporaryDirectory_;
    StoragePaths paths_;
    SqliteDatabase database_;
    CvRepository repository_;
    CvFileAccessService fileAccessService_;
    CvMutationQueue cvMutationQueue_;
    CvImportWorker importWorker_;
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
        document.originalFileName_ = QStringList{
            QStringLiteral("CV_Qt_2026.pdf"),
            QStringLiteral("CV_Embedded.pdf"),
            QStringLiteral("CV_General.pdf"),
            QStringLiteral("CV_Backend.pdf")}.at(index);
        document.storedFileName_ = QStringLiteral("stored-%1.pdf").arg(index);
        document.relativePath_ = QStringLiteral("Resumes/%1").arg(document.storedFileName_);
        document.sha256_ = QStringLiteral("test-hash-%1").arg(index);
        document.sizeBytes_ = 1024 + index;
        document.title_ = document.originalFileName_;
        document.category_ = index == 2 ? QStringLiteral("General") : QStringLiteral("Engineering");
        document.language_ = QStringLiteral("English");
        document.isFavorite_ = index == 0;
        document.createdAt_ = QDateTime{
            QDate{2026, 5, 12 - index},
            QTime{10, 0},
            Qt::UTC};
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
    void repositoryRoundTripPreservesTypedAndDisplayValues();
    void selectedCvControlsLinkedApplications();
    void favoriteToggleUpdatesSelectedCv();
    void favoriteFailureLeavesModelUnchanged();
    void fileAccessRejectsInvalidManagedPaths();
    void fileAccessRejectsTraversalPaths();
    void fileAccessRejectsMissingFiles();
    void openCvPublishesFileAccessFailure();
    void controllerFiltersAndSortsCvs();
    void activeArchivedViewsAndBulkSelectionFollowContracts();
    void selectionPublishesLinkedViewContracts();
    void importsMultipleBatchesInStrictFifo();
    void databaseLockFailureDoesNotBlockLaterImport();
    void cancelAllDropsQueuedImports();
    void cvPublicationIsIdempotent();
    void cvReplacementMovesApplicationLink_data();
    void cvReplacementMovesApplicationLink();
    void committedJobImportsRestoreArchiveRoles_data();
    void committedJobImportsRestoreArchiveRoles();
    void standaloneImportPublishesRestoredArchiveRoles();
    void failedCvInsertRollsBackAndContinuesFifo();
    void workerConnectionClosesOnShutdown();
    void permanentDeletionRetainsSkippedLinkedSelection();
    void mutationGateRejectsCvImportAdmission();
};

void CvLibraryControllerTest::cvModelExposesNamedRoles()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel;
    CvLibraryController controller(
        applicationsModel,
        makeCvDocuments(),
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_);
    const auto* model = controller.cvModel();

    QVERIFY(roleForName(*model, "id") > 0);
    QVERIFY(roleForName(*model, "fileName") > 0);
    QVERIFY(roleForName(*model, "categoryAccent") > 0);
    QVERIFY(roleForName(*model, "languageAccent") > 0);
    QVERIFY(roleForName(*model, "linkedApplicationCount") > 0);
    QVERIFY(roleForName(*model, "linkedApplicationCountLabel") > 0);
    QVERIFY(roleForName(*model, "isFavorite") > 0);
    QVERIFY(roleForName(*model, "createdAt") > 0);
    QVERIFY(roleForName(*model, "updatedAt") > 0);
    QVERIFY(roleForName(*model, "isArchived") > 0);
    QVERIFY(roleForName(*model, "archivedAt") > 0);
}

void CvLibraryControllerTest::cvModelExposesSeedDocuments()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel;
    CvLibraryController controller(
        applicationsModel,
        makeCvDocuments(),
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_);
    const auto* model = controller.cvModel();
    const auto firstRow = model->index(0, 0);

    QCOMPARE(model->rowCount(), 4);
    QCOMPARE(model->data(firstRow, roleForName(*model, "id")).toString(), QStringLiteral("cv-qt-2026"));
    QCOMPARE(model->data(firstRow, roleForName(*model, "fileName")).toString(), QStringLiteral("CV_Qt_2026.pdf"));
    QCOMPARE(model->data(firstRow, roleForName(*model, "linkedApplicationCount")).toInt(), 4);
    QCOMPARE(controller.resultSummary(), QStringLiteral("4 CVs"));
}

void CvLibraryControllerTest::repositoryRoundTripPreservesTypedAndDisplayValues()
{
    CvTestStorage storage;
    const auto sourceDocument = makeCvDocuments().first();
    storage.repository_.insert(sourceDocument);

    const auto storedDocuments = storage.repository_.findAll();
    QCOMPARE(storedDocuments.size(), 1);
    QCOMPARE(storedDocuments.first().createdAt_, sourceDocument.createdAt_);
    QCOMPARE(storedDocuments.first().updatedAt_, sourceDocument.updatedAt_);

    CvListModel model{storedDocuments};
    const auto row = model.index(0, 0);
    QCOMPARE(
        model.data(row, CvListModel::FileNameRole).toString(),
        QStringLiteral("CV_Qt_2026.pdf"));
    QCOMPARE(
        model.data(row, CvListModel::LastModifiedLabelRole).toString(),
        QStringLiteral("May 12, 2026"));
    QCOMPARE(
        model.data(row, CvListModel::FileSizeLabelRole).toString(),
        QStringLiteral("1 KB"));
    QCOMPARE(
        model.data(row, CvListModel::UpdatedAtRole).toDateTime(),
        storedDocuments.first().updatedAt_);
}

void CvLibraryControllerTest::selectedCvControlsLinkedApplications()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel(testsupport::makeJobApplications());
    CvLibraryController controller(
        applicationsModel,
        makeCvDocuments(),
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_);
    QSignalSpy selectedIndexSpy(&controller, &CvLibraryController::selectedCvIndexChanged);
    QSignalSpy selectedIdSpy(&controller, &CvLibraryController::selectedCvIdChanged);
    QSignalSpy selectedDataSpy(&controller, &CvLibraryController::selectedCvChanged);

    controller.selectCv(2);

    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedIdSpy.count(), 1);
    QCOMPARE(selectedDataSpy.count(), 1);
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-general"));
    const auto selected = controller.selectedCv();
    QCOMPARE(selected.value(QStringLiteral("fileName")).toString(), QStringLiteral("CV_General.pdf"));

    auto keys = selected.keys();
    QStringList expectedKeys{
        QStringLiteral("archivedAt"),
        QStringLiteral("category"),
        QStringLiteral("categoryAccent"),
        QStringLiteral("description"),
        QStringLiteral("fileName"),
        QStringLiteral("fileSizeLabel"),
        QStringLiteral("id"),
        QStringLiteral("isArchived"),
        QStringLiteral("isFavorite"),
        QStringLiteral("language"),
        QStringLiteral("languageAccent"),
        QStringLiteral("lastModifiedLabel"),
        QStringLiteral("linkedApplicationCount"),
        QStringLiteral("linkedApplicationCountLabel"),
        QStringLiteral("title"),
    };
    keys.sort();
    expectedKeys.sort();
    QCOMPARE(keys, expectedKeys);

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
        storage.fileAccessService_,
        storage.importWorker_);
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
        storage.fileAccessService_,
        storage.importWorker_);
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
        storage.fileAccessService_,
        storage.importWorker_);
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
        storage.fileAccessService_,
        storage.importWorker_);
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

    categorySummarySpy.clear();
    const auto categoryIndex = controller.cvListModel().index(0, 0);
    controller.cvListModel().dataChanged(
        categoryIndex,
        categoryIndex,
        {CvListModel::CategoryRole});
    QCOMPARE(categorySummarySpy.count(), 1);
    controller.cvListModel().dataChanged(
        categoryIndex,
        categoryIndex,
        {CvListModel::IsFavoriteRole});
    QCOMPARE(categorySummarySpy.count(), 1);
}

void CvLibraryControllerTest::activeArchivedViewsAndBulkSelectionFollowContracts()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel{testsupport::makeJobApplications()};
    auto documents = makeCvDocuments();
    documents[1].archivedAt_ = QDateTime::currentDateTimeUtc();
    CvLibraryController controller{
        applicationsModel,
        documents,
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_};
    QSignalSpy checkedSpy{&controller, &CvLibraryController::checkedCvsChanged};

    QCOMPARE(controller.libraryView(), CvLibraryController::LibraryView::Active);
    QCOMPARE(controller.cvCount(), 3);
    controller.setAllVisibleCvsChecked(true);
    QCOMPARE(controller.checkedCvCount(), 3);
    QCOMPARE(controller.checkedLinkedCvCount(), 2);
    QCOMPARE(controller.checkedUnlinkedCvCount(), 1);
    QVERIFY(controller.allVisibleCvsChecked());

    const auto checkedBeforeSort = controller.checkedCvIds();
    controller.setSortMode(QStringLiteral("File Name"));
    QCOMPARE(controller.checkedCvCount(), checkedBeforeSort.size());
    for (const auto& id : checkedBeforeSort) {
        QVERIFY(controller.checkedCvIds().contains(id));
    }
    controller.setCategoryFilter(QStringLiteral("General"));
    QCOMPARE(controller.checkedCvCount(), 0);

    controller.clearFilters();
    controller.setLibraryView(CvLibraryController::LibraryView::Archived);
    QCOMPARE(controller.cvCount(), 1);
    controller.setAllVisibleCvsChecked(true);
    QCOMPARE(controller.checkedCvCount(), 1);
    QCOMPARE(controller.checkedLinkedCvCount(), 1);
    QCOMPARE(controller.checkedUnlinkedCvCount(), 0);
    QVERIFY(!controller.canMutateCheckedCvs());

    controller.recordApplicationsDeleted({QStringLiteral("job-vision-embedded")});
    QCOMPARE(controller.checkedLinkedCvCount(), 0);
    QCOMPARE(controller.checkedUnlinkedCvCount(), 1);
    QCOMPARE(controller.cvCount(), 1);

    controller.setLibraryView(CvLibraryController::LibraryView::Active);
    QCOMPARE(controller.checkedCvCount(), 0);
    QVERIFY(checkedSpy.count() >= 4);
}

void CvLibraryControllerTest::selectionPublishesLinkedViewContracts()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel{testsupport::makeJobApplications()};
    CvLibraryController controller{
        applicationsModel,
        makeCvDocuments(),
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_};
    QSignalSpy selectedIndexSpy{&controller, &CvLibraryController::selectedCvIndexChanged};
    QSignalSpy selectedIdSpy{&controller, &CvLibraryController::selectedCvIdChanged};
    QSignalSpy selectedDataSpy{&controller, &CvLibraryController::selectedCvChanged};
    const auto* linkedModel = controller.linkedApplicationsModel();

    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-qt-2026"));
    QCOMPARE(linkedModel->rowCount(), 2);
    controller.setSortMode(QStringLiteral("File Name"));
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-qt-2026"));
    QCOMPARE(controller.selectedCvIndex(), 3);
    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedIdSpy.count(), 0);
    QCOMPARE(selectedDataSpy.count(), 0);
    QCOMPARE(linkedModel->rowCount(), 2);

    selectedIndexSpy.clear();
    selectedIdSpy.clear();
    selectedDataSpy.clear();
    QSignalSpy categorySummarySpy{&controller, &CvLibraryController::categorySummaryChanged};
    QSignalSpy countSpy{&controller, &CvLibraryController::cvCountChanged};
    QSignalSpy resultSummarySpy{&controller, &CvLibraryController::resultSummaryChanged};
    auto insertedDocument = makeCvDocuments().first();
    insertedDocument.id_ = QStringLiteral("cv-alphabetical-first");
    insertedDocument.originalFileName_ = QStringLiteral("AAA_CV.pdf");
    insertedDocument.storedFileName_ = QStringLiteral("stored-alphabetical-first.pdf");
    insertedDocument.relativePath_ = QStringLiteral("Resumes/stored-alphabetical-first.pdf");
    insertedDocument.sha256_ = QStringLiteral("test-hash-alphabetical-first");
    insertedDocument.linkedApplicationIds_.clear();

    controller.recordCvUse(
        insertedDocument,
        QStringLiteral("job-alphabetical-first"),
        CvImportDisposition::Inserted);

    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-qt-2026"));
    QCOMPARE(controller.selectedCvIndex(), 4);
    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedIdSpy.count(), 0);
    QCOMPARE(selectedDataSpy.count(), 0);
    QCOMPARE(linkedModel->rowCount(), 2);
    QCOMPARE(categorySummarySpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(resultSummarySpy.count(), 1);
}

void CvLibraryControllerTest::importsMultipleBatchesInStrictFifo()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel;
    CvLibraryController controller{
        applicationsModel,
        {},
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_};
    QSignalSpy completedSpy{&controller, &CvLibraryController::cvImportCompleted};
    QSignalSpy pendingSpy{&controller, &CvLibraryController::pendingImportCountChanged};
    QSignalSpy importingSpy{&controller, &CvLibraryController::importingChanged};
    QSignalSpy drainedSpy{&controller, &CvLibraryController::importQueueDrained};

    const auto firstPath = storage.createSourceFile(
        QStringLiteral("first.pdf"),
        QByteArrayLiteral("%PDF-1.4 First"));
    const auto invalidPath = storage.createSourceFile(
        QStringLiteral("invalid.txt"),
        QByteArrayLiteral("not a CV"));
    const auto lastPath = storage.createSourceFile(
        QStringLiteral("last.docx"),
        QByteArrayLiteral("DOCX Last"));
    QVERIFY(!firstPath.isEmpty());
    QVERIFY(!invalidPath.isEmpty());
    QVERIFY(!lastPath.isEmpty());

    controller.addCvs({
        QUrl::fromLocalFile(firstPath),
        QUrl::fromLocalFile(firstPath),
    });
    controller.addCvs({
        QUrl::fromLocalFile(invalidPath),
        QUrl::fromLocalFile(lastPath),
    });

    QCOMPARE(controller.pendingImportCount(), 4);
    QVERIFY(controller.importing());
    QCOMPARE(pendingSpy.count(), 2);
    QCOMPARE(importingSpy.count(), 1);

    QTRY_COMPARE_WITH_TIMEOUT(drainedSpy.count(), 1, 15000);
    QCOMPARE(completedSpy.count(), 4);
    QCOMPARE(controller.pendingImportCount(), 0);
    QVERIFY(!controller.importing());
    QCOMPARE(pendingSpy.count(), 6);
    QCOMPARE(importingSpy.count(), 2);

    const QStringList expectedFileNames{
        QStringLiteral("first.pdf"),
        QStringLiteral("first.pdf"),
        QStringLiteral("invalid.txt"),
        QStringLiteral("last.docx"),
    };
    for (int index = 0; index < completedSpy.count(); ++index) {
        QCOMPARE(completedSpy.at(index).at(0).toULongLong(), static_cast<quint64>(index + 1));
        QCOMPARE(completedSpy.at(index).at(1).toString(), expectedFileNames.at(index));
        QVERIFY(!completedSpy.at(index).at(4).toString().isEmpty());
    }
    QVERIFY(completedSpy.at(0).at(2).toBool());
    QCOMPARE(completedSpy.at(0).at(3).toString(), QStringLiteral("inserted"));
    QVERIFY(completedSpy.at(1).at(2).toBool());
    QCOMPARE(completedSpy.at(1).at(3).toString(), QStringLiteral("existing-active"));
    QVERIFY(!completedSpy.at(2).at(2).toBool());
    QCOMPARE(completedSpy.at(2).at(3).toString(), QStringLiteral("existing-active"));
    QVERIFY(completedSpy.at(3).at(2).toBool());
    QCOMPARE(completedSpy.at(3).at(3).toString(), QStringLiteral("inserted"));

    QCOMPARE(storage.repository_.findAll().size(), 2);
    QCOMPARE(controller.cvListModel().rowCount(), 2);
    const QDir resumesDirectory{storage.paths_.resumesDirectory()};
    QCOMPARE(resumesDirectory.entryList(QDir::Files | QDir::NoDotAndDotDot).size(), 2);
    QVERIFY(resumesDirectory.entryList(
        {QStringLiteral("*.part")},
        QDir::Files | QDir::NoDotAndDotDot).isEmpty());
}

void CvLibraryControllerTest::databaseLockFailureDoesNotBlockLaterImport()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel;
    CvLibraryController controller{
        applicationsModel,
        {},
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_};
    QSignalSpy completedSpy{&controller, &CvLibraryController::cvImportCompleted};
    QSignalSpy drainedSpy{&controller, &CvLibraryController::importQueueDrained};

    const auto warmPath = storage.createSourceFile(QStringLiteral("warm.pdf"));
    QVERIFY(!warmPath.isEmpty());
    controller.addCvs({QUrl::fromLocalFile(warmPath)});
    QTRY_COMPARE_WITH_TIMEOUT(drainedSpy.count(), 1, 10000);
    QCOMPARE(completedSpy.count(), 1);
    QVERIFY(completedSpy.first().at(2).toBool());
    completedSpy.clear();
    drainedSpy.clear();

    QSqlQuery exclusiveLock{storage.database_.connection()};
    QVERIFY(exclusiveLock.exec(QStringLiteral("BEGIN EXCLUSIVE")));
    bool lockReleased = false;
    QObject::connect(
        &controller,
        &CvLibraryController::cvImportCompleted,
        &controller,
        [&storage, &lockReleased](
            quint64,
            const QString& fileName,
            bool,
            const QString&,
            const QString&) {
            if (fileName != QStringLiteral("locked.pdf")) {
                return;
            }
            QSqlQuery rollback{storage.database_.connection()};
            lockReleased = rollback.exec(QStringLiteral("ROLLBACK"));
        });

    const auto lockedPath = storage.createSourceFile(QStringLiteral("locked.pdf"));
    const auto afterLockPath = storage.createSourceFile(QStringLiteral("after-lock.pdf"));
    QVERIFY(!lockedPath.isEmpty());
    QVERIFY(!afterLockPath.isEmpty());
    controller.addCvs({
        QUrl::fromLocalFile(lockedPath),
        QUrl::fromLocalFile(afterLockPath),
    });

    QTRY_COMPARE_WITH_TIMEOUT(drainedSpy.count(), 1, 10000);
    QCOMPARE(completedSpy.count(), 2);
    QCOMPARE(completedSpy.at(0).at(1).toString(), QStringLiteral("locked.pdf"));
    QVERIFY(!completedSpy.at(0).at(2).toBool());
    QVERIFY(lockReleased);
    QCOMPARE(completedSpy.at(1).at(1).toString(), QStringLiteral("after-lock.pdf"));
    QVERIFY(completedSpy.at(1).at(2).toBool());
    QCOMPARE(completedSpy.at(1).at(3).toString(), QStringLiteral("inserted"));
    QCOMPARE(storage.repository_.findAll().size(), 2);
    QCOMPARE(controller.cvListModel().rowCount(), 2);
    const QDir resumesDirectory{storage.paths_.resumesDirectory()};
    QCOMPARE(resumesDirectory.entryList(QDir::Files | QDir::NoDotAndDotDot).size(), 2);
    QVERIFY(resumesDirectory.entryList(
        {QStringLiteral("*.part")},
        QDir::Files | QDir::NoDotAndDotDot).isEmpty());
}

void CvLibraryControllerTest::cancelAllDropsQueuedImports()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel;
    CvLibraryController controller{
        applicationsModel,
        {},
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_};
    QSignalSpy completedSpy{&controller, &CvLibraryController::cvImportCompleted};
    QSignalSpy drainedSpy{&controller, &CvLibraryController::importQueueDrained};

    const auto missingPath = QDir{storage.temporaryDirectory_.path()}
        .filePath(QStringLiteral("missing.pdf"));
    const auto queuedFirst = storage.createSourceFile(QStringLiteral("queued-first.pdf"));
    const auto queuedSecond = storage.createSourceFile(QStringLiteral("queued-second.pdf"));
    QVERIFY(!queuedFirst.isEmpty());
    QVERIFY(!queuedSecond.isEmpty());

    controller.addCvs({
        QUrl::fromLocalFile(missingPath),
        QUrl::fromLocalFile(queuedFirst),
        QUrl::fromLocalFile(queuedSecond),
    });
    controller.cancelAllCvImports();

    QCOMPARE(controller.pendingImportCount(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(drainedSpy.count(), 1, 10000);
    QCOMPARE(controller.pendingImportCount(), 0);
    QVERIFY(!controller.importing());
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(storage.repository_.findAll().size(), 0);
    QCOMPARE(controller.cvListModel().rowCount(), 0);
    const QDir resumesDirectory{storage.paths_.resumesDirectory()};
    QVERIFY(resumesDirectory.entryList(QDir::Files | QDir::NoDotAndDotDot).isEmpty());
}

void CvLibraryControllerTest::cvPublicationIsIdempotent()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel;
    CvLibraryController controller{
        applicationsModel,
        {},
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_};
    QSignalSpy insertedSpy{&controller.cvListModel(), &QAbstractItemModel::rowsInserted};
    auto document = makeCvDocuments().first();
    document.linkedApplicationIds_.clear();

    controller.recordCvUse(document, QStringLiteral("job-1"), CvImportDisposition::Inserted);
    controller.recordCvUse(document, QStringLiteral("job-1"), CvImportDisposition::ExistingActive);
    controller.recordCvUse(document, QStringLiteral("job-2"), CvImportDisposition::ExistingActive);

    QCOMPARE(insertedSpy.count(), 1);
    QCOMPARE(controller.cvListModel().rowCount(), 1);
    const auto* published = controller.cvListModel().cvAt(0);
    QVERIFY(published != nullptr);
    QCOMPARE(
        published->linkedApplicationIds_,
        QStringList({QStringLiteral("job-1"), QStringLiteral("job-2")}));
}

void CvLibraryControllerTest::cvReplacementMovesApplicationLink_data()
{
    QTest::addColumn<bool>("existingReplacement");
    QTest::newRow("existing-cv") << true;
    QTest::newRow("inserted-cv") << false;
}

void CvLibraryControllerTest::cvReplacementMovesApplicationLink()
{
    QFETCH(bool, existingReplacement);
    CvTestStorage storage;
    JobApplicationListModel applicationsModel;
    auto documents = makeCvDocuments();
    documents.resize(2);
    const auto applicationId = QStringLiteral("job-relinked");
    documents[0].linkedApplicationIds_ = {applicationId};
    documents[1].linkedApplicationIds_.clear();
    const auto previousCvId = documents[0].id_;
    const auto replacementCvId = documents[1].id_;
    const auto replacementDocument = documents[1];
    if (!existingReplacement) {
        documents.removeLast();
    }
    CvLibraryController controller{
        applicationsModel,
        documents,
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_};

    QStringList publicationOrder;
    connect(&controller.cvListModel(), &QAbstractItemModel::dataChanged, &controller,
        [&](const QModelIndex& first, const QModelIndex&, const QList<int>& roles) {
            if (roles.contains(CvListModel::LinkedApplicationCountRole)) {
                publicationOrder.append(controller.cvListModel().cvAt(first.row())->id_);
            }
        });
    connect(&controller.cvListModel(), &QAbstractItemModel::rowsInserted, &controller,
        [&](const QModelIndex&, int first, int) {
            publicationOrder.append(controller.cvListModel().cvAt(first)->id_);
        });

    controller.recordCvReplacement(
        previousCvId,
        replacementDocument,
        applicationId,
        existingReplacement ? CvImportDisposition::ExistingActive : CvImportDisposition::Inserted);

    QCOMPARE(publicationOrder, QStringList({previousCvId, replacementCvId}));
    const CvDocument* previous = nullptr;
    const CvDocument* replacement = nullptr;
    for (int row = 0; row < controller.cvListModel().rowCount(); ++row) {
        const auto* document = controller.cvListModel().cvAt(row);
        if (document != nullptr && document->id_ == previousCvId)
            previous = document;
        if (document != nullptr && document->id_ == replacementCvId)
            replacement = document;
    }
    QVERIFY(previous != nullptr);
    QVERIFY(replacement != nullptr);
    QVERIFY(!previous->linkedApplicationIds_.contains(applicationId));
    QVERIFY(replacement->linkedApplicationIds_.contains(applicationId));
}

void CvLibraryControllerTest::committedJobImportsRestoreArchiveRoles_data()
{
    QTest::addColumn<int>("operation");
    QTest::newRow("add-job") << 0;
    QTest::newRow("replace-different-id") << 1;
    QTest::newRow("replace-same-id") << 2;
}

void CvLibraryControllerTest::committedJobImportsRestoreArchiveRoles()
{
    QFETCH(int, operation);
    testsupport::AddJobWorkerTestFixture fixture;
    const auto source = QUrl::fromLocalFile(fixture.storage_.createFile());
    const auto original = fixture.service_.create(testsupport::validJobDraft(), source);
    QVERIFY(original.success_);
    auto target = original;
    if (operation == 1) {
        target = fixture.service_.create(testsupport::validJobDraft(),
            QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("previous.pdf"))));
        QVERIFY(target.success_);
    }
    QVERIFY(fixture.cvRepository_.updateArchived(original.cvDocument_.id_, true).has_value());
    JobApplicationsController jobs{fixture.jobRepository_.findAll(), fixture.worker_};
    CvFileAccessService access{fixture.storage_.paths()};
    CvImportWorker importWorker{fixture.storage_.paths().dataDirectory(), fixture.cvMutationQueue_};
    CvLibraryController cvs{jobs.jobApplicationListModel(), fixture.cvRepository_.findAll(),
        fixture.cvRepository_, access, importWorker};
    connect(&jobs, &JobApplicationsController::cvUsed, &cvs, &CvLibraryController::recordCvUse);
    connect(&jobs, &JobApplicationsController::cvReplaced, &cvs, &CvLibraryController::recordCvReplacement);
    QSignalSpy added{&jobs, &JobApplicationsController::applicationSaveCompleted};
    QSignalSpy updated{&jobs, &JobApplicationsController::applicationUpdateCompleted};
    QSignalSpy changed{&cvs.cvListModel(), &QAbstractItemModel::dataChanged};
    const auto visibleBefore = cvs.cvModel()->rowCount();
    QString applicationId;
    if (operation == 0) {
        jobs.createApplication(testsupport::validJobFormValues(), source);
        QTRY_COMPARE_WITH_TIMEOUT(added.count(), 1, 10000);
        QVERIFY(added.first().at(2).toBool());
        const auto persisted = fixture.jobRepository_.findAll();
        for (const auto& application : persisted) {
            if (application.id_ != original.application_.id_) {
                applicationId = application.id_;
            }
        }
    } else {
        applicationId = target.application_.id_;
        jobs.updateApplication(applicationId, testsupport::validJobFormValues(), source);
        QTRY_COMPARE_WITH_TIMEOUT(updated.count(), 1, 10000);
        QVERIFY(updated.first().at(3).toBool());
    }
    QVERIFY(!applicationId.isEmpty());
    const auto* published = cvs.cvListModel().cvById(original.cvDocument_.id_);
    QVERIFY(published != nullptr);
    QVERIFY(!published->archivedAt_.isValid());
    QVERIFY(published->linkedApplicationIds_.contains(applicationId));
    QCOMPARE(published->linkedApplicationIds_.count(applicationId), 1);
    QCOMPARE(cvs.cvModel()->rowCount(), visibleBefore + 1);
    QVERIFY(!changed.isEmpty());
    QVERIFY(changed.first().at(2).value<QList<int>>().contains(CvListModel::IsArchivedRole));
    if (operation == 1) {
        QVERIFY(!cvs.cvListModel().cvById(target.cvDocument_.id_)->linkedApplicationIds_.contains(applicationId));
    }
}

void CvLibraryControllerTest::standaloneImportPublishesRestoredArchiveRoles()
{
    CvTestStorage storage;
    JobApplicationListModel applications;
    CvLibraryController controller{applications, {}, storage.repository_,
        storage.fileAccessService_, storage.importWorker_};
    QSignalSpy completed{&controller, &CvLibraryController::cvImportCompleted};
    const auto source = QUrl::fromLocalFile(storage.createSourceFile(QStringLiteral("restore.pdf")));
    controller.addCvs({source});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 10000);
    const auto document = storage.repository_.findAll().first();
    const auto archivedAt = storage.repository_.updateArchived(document.id_, true);
    QVERIFY(archivedAt.has_value());
    controller.cvListModel().setArchiveState(document.id_, *archivedAt, *archivedAt);
    QCOMPARE(controller.cvModel()->rowCount(), 0);

    controller.addCvs({source});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 2, 10000);
    QCOMPARE(completed.last().at(3).toString(), QStringLiteral("restored-archived"));
    QCOMPARE(controller.cvModel()->rowCount(), 1);
    QVERIFY(!controller.cvListModel().cvById(document.id_)->archivedAt_.isValid());
    QCOMPARE(QDir{storage.paths_.resumesDirectory()}.entryList(QDir::Files).size(), 1);
}

void CvLibraryControllerTest::failedCvInsertRollsBackAndContinuesFifo()
{
    CvTestStorage storage;
    QSqlQuery trigger{storage.database_.connection()};
    QVERIFY(trigger.exec(QStringLiteral(
        "CREATE TRIGGER reject_one_cv BEFORE INSERT ON cvs "
        "WHEN NEW.original_file_name = 'reject.pdf' "
        "BEGIN SELECT RAISE(FAIL, 'forced CV insert failure'); END")));
    JobApplicationListModel applications;
    CvLibraryController controller{applications, {}, storage.repository_,
        storage.fileAccessService_, storage.importWorker_};
    QSignalSpy completed{&controller, &CvLibraryController::cvImportCompleted};
    controller.addCvs({
        QUrl::fromLocalFile(storage.createSourceFile(QStringLiteral("reject.pdf"))),
        QUrl::fromLocalFile(storage.createSourceFile(QStringLiteral("keep.pdf")))});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 2, 10000);
    QVERIFY(!completed.first().at(2).toBool());
    QVERIFY(completed.last().at(2).toBool());
    QCOMPARE(storage.repository_.findAll().size(), 1);
    QCOMPARE(controller.cvListModel().rowCount(), 1);
    QCOMPARE(QDir{storage.paths_.resumesDirectory()}.entryList(QDir::Files).size(), 1);
    QVERIFY(QDir{storage.paths_.resumesDirectory()}.entryList({QStringLiteral("*.part")}, QDir::Files).isEmpty());
}

void CvLibraryControllerTest::workerConnectionClosesOnShutdown()
{
    const auto initialConnectionCount = QSqlDatabase::connectionNames().size();
    {
        CvTestStorage storage;
        JobApplicationListModel applicationsModel;
        CvLibraryController controller{
            applicationsModel,
            {},
            storage.repository_,
            storage.fileAccessService_,
            storage.importWorker_};
        QSignalSpy drainedSpy{&controller, &CvLibraryController::importQueueDrained};
        const auto sourcePath = storage.createSourceFile(QStringLiteral("shutdown.pdf"));
        QVERIFY(!sourcePath.isEmpty());

        controller.addCvs({QUrl::fromLocalFile(sourcePath)});
        QTRY_COMPARE_WITH_TIMEOUT(drainedSpy.count(), 1, 10000);
        QVERIFY(storage.importWorker_.isRunning());
    }

    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);
}

void CvLibraryControllerTest::permanentDeletionRetainsSkippedLinkedSelection()
{
    CvTestStorage storage;
    CvManagedFileStore fileStore{storage.paths_};
    CvImportService importer{fileStore, storage.repository_};
    const auto cancellation = std::make_shared<CancellationState>();
    const auto linkedSource = storage.createSourceFile(
        QStringLiteral("linked-archived.pdf"),
        QByteArrayLiteral("%PDF linked"));
    const auto unlinkedSource = storage.createSourceFile(
        QStringLiteral("unlinked-archived.pdf"),
        QByteArrayLiteral("%PDF unlinked"));
    auto preparation = importer.prepareDocument(QUrl::fromLocalFile(linkedSource), cancellation);
    QVERIFY(preparation.succeeded());
    auto lease = storage.cvMutationQueue_.acquire(cancellation);
    SqlTransaction transaction{storage.database_.connection(), QStringLiteral("Seed archived CVs")};
    const auto linked = importer.importPreparedDocument(preparation.preparation_, cancellation);
    preparation = importer.prepareDocument(QUrl::fromLocalFile(unlinkedSource), cancellation);
    QVERIFY(preparation.succeeded());
    const auto unlinked = importer.importPreparedDocument(preparation.preparation_, cancellation);
    transaction.commit();
    lease = {};

    const auto timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    QSqlQuery insert{storage.database_.connection()};
    insert.prepare(QStringLiteral(
        "INSERT INTO companies (id, display_name, normalized_name, created_at, updated_at) "
        "VALUES ('linked-company', 'Linked Company', 'linked company', ?, ?)"));
    insert.addBindValue(timestamp);
    insert.addBindValue(timestamp);
    QVERIFY(insert.exec());
    insert.prepare(QStringLiteral(
        "INSERT INTO jobs (id, company_id, job_title, status, applied_date, cv_id, created_at, updated_at) "
        "VALUES ('linked-job', 'linked-company', 'Linked Role', 'Applied', '2026-08-19', ?, ?, ?)"));
    insert.addBindValue(linked.document_.id_);
    insert.addBindValue(timestamp);
    insert.addBindValue(timestamp);
    QVERIFY(insert.exec());
    QVERIFY(storage.repository_.updateArchived(linked.document_.id_, true).has_value());
    QVERIFY(storage.repository_.updateArchived(unlinked.document_.id_, true).has_value());

    JobRepository jobs{storage.database_.connection()};
    JobApplicationListModel applicationsModel{jobs.findAll()};
    StorageMutationGate gate;
    DataRemovalWorker removalWorker{storage.paths_.dataDirectory()};
    CvLibraryController controller{
        applicationsModel,
        storage.repository_.findAll(),
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_,
        removalWorker,
        gate};
    QSignalSpy completedSpy{&controller, &CvLibraryController::cvMutationCompleted};

    controller.setLibraryView(CvLibraryController::LibraryView::Archived);
    controller.setAllVisibleCvsChecked(true);
    QCOMPARE(controller.checkedCvCount(), 2);
    QCOMPARE(controller.checkedLinkedCvCount(), 1);
    QCOMPARE(controller.checkedUnlinkedCvCount(), 1);
    controller.permanentlyDeleteCheckedCvs();

    QTRY_COMPARE_WITH_TIMEOUT(completedSpy.count(), 1, 10000);
    QCOMPARE(completedSpy.first().at(0).toInt(), 1);
    QCOMPARE(completedSpy.first().at(3).toInt(), 1);
    QCOMPARE(completedSpy.first().at(4).toInt(), 0);
    QCOMPARE(controller.checkedCvIds(), QStringList{linked.document_.id_});
    QCOMPARE(controller.cvCount(), 1);
    QVERIFY(storage.repository_.findById(linked.document_.id_).has_value());
    QVERIFY(!storage.repository_.findById(unlinked.document_.id_).has_value());
    QVERIFY(QFileInfo::exists(linked.completedFilePath_));
    QVERIFY(!QFileInfo::exists(unlinked.completedFilePath_));
}

void CvLibraryControllerTest::mutationGateRejectsCvImportAdmission()
{
    CvTestStorage storage;
    JobApplicationListModel applicationsModel;
    StorageMutationGate gate;
    DataRemovalWorker removalWorker{storage.paths_.dataDirectory()};
    CvLibraryController controller{
        applicationsModel,
        {},
        storage.repository_,
        storage.fileAccessService_,
        storage.importWorker_,
        removalWorker,
        gate};
    QSignalSpy failedSpy{&controller, &CvLibraryController::operationFailed};
    QSignalSpy completedSpy{&controller, &CvLibraryController::cvImportCompleted};
    const auto sourcePath = storage.createSourceFile(QStringLiteral("blocked-import.pdf"));

    QVERIFY(gate.beginRemoval());
    controller.addCvs({QUrl::fromLocalFile(sourcePath)});

    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(completedSpy.count(), 0);
    QCOMPARE(controller.pendingImportCount(), 0);
    QCOMPARE(storage.repository_.findAll().size(), 0);
    gate.endRemoval();
}

QTEST_GUILESS_MAIN(CvLibraryControllerTest)

#include "CvLibraryControllerTest.moc"

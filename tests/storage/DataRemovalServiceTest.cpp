#include "../support/AddJobTestFixture.hpp"

#include "common/CancellationState.hpp"
#include "maintenance/DataRemovalService.hpp"
#include "maintenance/DataRemovalWorker.hpp"
#include "maintenance/StorageMutationGate.hpp"
#include "storage/SqlTransaction.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUrl>
#include <QtTest/QtTest>

#include <memory>

namespace {

CvImportResult importCv(
    testsupport::AddJobTestFixture& fixture,
    const QString& fileName,
    const QByteArray& contents = QByteArrayLiteral("%PDF-1.4 removal test"))
{
    const auto sourcePath = fixture.storage_.createFile(fileName, contents);
    const auto cancellation = std::make_shared<CancellationState>();
    const auto preparation = fixture.importer_.prepareDocument(
        QUrl::fromLocalFile(sourcePath),
        cancellation);
    if (!preparation.succeeded()) {
        return {};
    }
    auto lease = fixture.cvMutationQueue_.acquire(cancellation);
    SqlTransaction transaction{fixture.database_.connection(), QStringLiteral("Seed removal CV")};
    auto result = fixture.importer_.importPreparedDocument(preparation.preparation_, cancellation);
    transaction.commit();
    return result;
}

DataRemovalService removalService(testsupport::AddJobTestFixture& fixture)
{
    return {
        fixture.database_.connection(),
        fixture.jobRepository_,
        fixture.cvRepository_,
        fixture.fileStore_};
}

QVector<DataRemovalItemRequest> requestFor(const QString& id, const QString& label = {})
{
    return {{id, label.isEmpty() ? id : label}};
}

}

class DataRemovalServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void deletingJobCascadesTechnologiesAndPreservesCompanyAndCv();
    void removingLinkedActiveCvArchivesWithoutRemovingFile();
    void removingUnlinkedActiveCvDeletesRecordAndFile();
    void permanentDeletionSkipsLinkedArchivedCvAndDeletesEligibleCv();
    void restorePreservesIdentityMetadataFavoriteLinksAndFile();
    void itemFailureDoesNotStopLaterItems();
    void rejectsUnsafePathsAndDeletesRowsForMissingFiles();
    void restoresFileWhenDatabaseDeleteFails();
    void recoversReferencedAndCommittedTombstones();
    void rejectsSymlinkWhenSupported();
    void mutationGateSerializesAddAndRemovalWork();
    void workerShutdownCancelsActiveWorkAndClosesItsConnection();
};

void DataRemovalServiceTest::deletingJobCascadesTechnologiesAndPreservesCompanyAndCv()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto created = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(fixture.storage_.createFile()));
    QVERIFY2(created.success_, qPrintable(created.message_));

    auto service = removalService(fixture);
    const auto result = service.process(
        DataRemovalKind::DeleteJobs,
        requestFor(created.application_.id_, created.application_.jobTitle_),
        std::make_shared<CancellationState>());

    QCOMPARE(result.items_.size(), 1);
    QCOMPARE(result.items_.first().status_, DataRemovalItemStatus::Deleted);
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QCOMPARE(fixture.cvRepository_.findAll().size(), 1);
    QCOMPARE(fixture.companyRepository_.findAll().size(), 1);
    QSqlQuery technologies{fixture.database_.connection()};
    QVERIFY(technologies.exec(QStringLiteral("SELECT COUNT(*) FROM job_technologies")));
    QVERIFY(technologies.next());
    QCOMPARE(technologies.value(0).toInt(), 0);
}

void DataRemovalServiceTest::removingLinkedActiveCvArchivesWithoutRemovingFile()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto created = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(fixture.storage_.createFile()));
    QVERIFY(created.success_);
    const auto managedPath = QDir{fixture.storage_.paths().dataDirectory()}
        .filePath(created.cvDocument_.relativePath_);

    auto service = removalService(fixture);
    const auto result = service.process(
        DataRemovalKind::RemoveActiveCvs,
        requestFor(created.cvDocument_.id_),
        std::make_shared<CancellationState>());

    QCOMPARE(result.items_.first().status_, DataRemovalItemStatus::Archived);
    const auto stored = fixture.cvRepository_.findById(created.cvDocument_.id_);
    QVERIFY(stored.has_value());
    QVERIFY(stored->archivedAt_.isValid());
    QCOMPARE(result.items_.first().document_.archivedAt_, stored->archivedAt_);
    QCOMPARE(result.items_.first().document_.updatedAt_, stored->updatedAt_);
    QCOMPARE(stored->linkedApplicationIds_, QStringList{created.application_.id_});
    QVERIFY(QFileInfo::exists(managedPath));
}

void DataRemovalServiceTest::removingUnlinkedActiveCvDeletesRecordAndFile()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto imported = importCv(fixture, QStringLiteral("unlinked.pdf"));
    QCOMPARE(imported.disposition_, CvImportDisposition::Inserted);
    QVERIFY(QFileInfo::exists(imported.completedFilePath_));

    auto service = removalService(fixture);
    const auto result = service.process(
        DataRemovalKind::RemoveActiveCvs,
        requestFor(imported.document_.id_),
        std::make_shared<CancellationState>());

    QCOMPARE(result.items_.first().status_, DataRemovalItemStatus::Deleted);
    QVERIFY(!fixture.cvRepository_.findById(imported.document_.id_).has_value());
    QVERIFY(!QFileInfo::exists(imported.completedFilePath_));
    QVERIFY(!QFileInfo::exists(imported.completedFilePath_ + QStringLiteral(".delete")));
}

void DataRemovalServiceTest::permanentDeletionSkipsLinkedArchivedCvAndDeletesEligibleCv()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto linked = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("linked.pdf"))));
    QVERIFY(linked.success_);
    const auto unlinked = importCv(fixture, QStringLiteral("unlinked.pdf"));
    QCOMPARE(unlinked.disposition_, CvImportDisposition::Inserted);
    QVERIFY(fixture.cvRepository_.updateArchived(linked.cvDocument_.id_, true).has_value());
    QVERIFY(fixture.cvRepository_.updateArchived(unlinked.document_.id_, true).has_value());

    auto service = removalService(fixture);
    const QVector<DataRemovalItemRequest> requests{
        {linked.cvDocument_.id_, QStringLiteral("linked.pdf")},
        {unlinked.document_.id_, QStringLiteral("unlinked.pdf")},
    };
    const auto result = service.process(
        DataRemovalKind::DeleteArchivedCvs,
        requests,
        std::make_shared<CancellationState>());

    QCOMPARE(result.items_.size(), 2);
    QCOMPARE(result.items_.at(0).status_, DataRemovalItemStatus::SkippedLinked);
    QCOMPARE(result.items_.at(1).status_, DataRemovalItemStatus::Deleted);
    QVERIFY(fixture.cvRepository_.findById(linked.cvDocument_.id_).has_value());
    QVERIFY(!fixture.cvRepository_.findById(unlinked.document_.id_).has_value());
}

void DataRemovalServiceTest::restorePreservesIdentityMetadataFavoriteLinksAndFile()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto created = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(fixture.storage_.createFile(QStringLiteral("restore.pdf"))));
    QVERIFY(created.success_);
    QVERIFY(fixture.cvRepository_.updateFavorite(created.cvDocument_.id_, true).has_value());
    QVERIFY(fixture.cvRepository_.updateArchived(created.cvDocument_.id_, true).has_value());
    const auto before = fixture.cvRepository_.findById(created.cvDocument_.id_);
    QVERIFY(before.has_value());
    const auto managedPath = QDir{fixture.storage_.paths().dataDirectory()}
        .filePath(before->relativePath_);

    auto service = removalService(fixture);
    const auto result = service.process(
        DataRemovalKind::RestoreArchivedCvs,
        requestFor(before->id_),
        std::make_shared<CancellationState>());

    QCOMPARE(result.items_.first().status_, DataRemovalItemStatus::Restored);
    const auto after = fixture.cvRepository_.findById(before->id_);
    QVERIFY(after.has_value());
    QCOMPARE(after->id_, before->id_);
    QCOMPARE(after->sha256_, before->sha256_);
    QCOMPARE(after->originalFileName_, before->originalFileName_);
    QCOMPARE(after->relativePath_, before->relativePath_);
    QCOMPARE(after->linkedApplicationIds_, before->linkedApplicationIds_);
    QVERIFY(after->isFavorite_);
    QVERIFY(!after->archivedAt_.isValid());
    QVERIFY(!result.items_.first().document_.archivedAt_.isValid());
    QCOMPARE(result.items_.first().document_.updatedAt_, after->updatedAt_);
    QVERIFY(QFileInfo::exists(managedPath));
}

void DataRemovalServiceTest::itemFailureDoesNotStopLaterItems()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto created = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(fixture.storage_.createFile()));
    QVERIFY(created.success_);

    auto service = removalService(fixture);
    const QVector<DataRemovalItemRequest> requests{
        {QStringLiteral("missing-job"), QStringLiteral("Missing")},
        {created.application_.id_, created.application_.jobTitle_},
    };
    const auto result = service.process(
        DataRemovalKind::DeleteJobs,
        requests,
        std::make_shared<CancellationState>());

    QCOMPARE(result.items_.size(), 2);
    QCOMPARE(result.items_.at(0).status_, DataRemovalItemStatus::Failed);
    QCOMPARE(result.items_.at(1).status_, DataRemovalItemStatus::Deleted);
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
}

void DataRemovalServiceTest::rejectsUnsafePathsAndDeletesRowsForMissingFiles()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto outsidePath = fixture.storage_.createFile(QStringLiteral("outside.pdf"));
    CvDocument unsafe;
    unsafe.id_ = QStringLiteral("unsafe-cv");
    unsafe.originalFileName_ = QStringLiteral("outside.pdf");
    unsafe.storedFileName_ = QStringLiteral("outside.pdf");
    unsafe.relativePath_ = QStringLiteral("../outside.pdf");
    unsafe.sha256_ = QStringLiteral("unsafe-hash");
    unsafe.sizeBytes_ = 1;
    unsafe.createdAt_ = QDateTime::currentDateTimeUtc();
    unsafe.updatedAt_ = unsafe.createdAt_;
    fixture.cvRepository_.insert(unsafe);

    const auto missing = importCv(fixture, QStringLiteral("missing.pdf"));
    QCOMPARE(missing.disposition_, CvImportDisposition::Inserted);
    QVERIFY(QFile::remove(missing.completedFilePath_));

    auto service = removalService(fixture);
    const QVector<DataRemovalItemRequest> requests{
        {unsafe.id_, QStringLiteral("Unsafe")},
        {missing.document_.id_, QStringLiteral("Missing file")},
    };
    const auto result = service.process(
        DataRemovalKind::RemoveActiveCvs,
        requests,
        std::make_shared<CancellationState>());

    QCOMPARE(result.items_.at(0).status_, DataRemovalItemStatus::Failed);
    QCOMPARE(result.items_.at(1).status_, DataRemovalItemStatus::Deleted);
    QVERIFY(fixture.cvRepository_.findById(unsafe.id_).has_value());
    QVERIFY(!fixture.cvRepository_.findById(missing.document_.id_).has_value());
    QVERIFY(QFileInfo::exists(outsidePath));
}

void DataRemovalServiceTest::restoresFileWhenDatabaseDeleteFails()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto imported = importCv(fixture, QStringLiteral("compensate.pdf"));
    QCOMPARE(imported.disposition_, CvImportDisposition::Inserted);
    QSqlQuery trigger{fixture.database_.connection()};
    QVERIFY(trigger.exec(QStringLiteral(
        "CREATE TRIGGER reject_cv_delete BEFORE DELETE ON cvs "
        "BEGIN SELECT RAISE(ABORT, 'test delete failure'); END")));

    auto service = removalService(fixture);
    const auto result = service.process(
        DataRemovalKind::RemoveActiveCvs,
        requestFor(imported.document_.id_),
        std::make_shared<CancellationState>());

    QCOMPARE(result.items_.first().status_, DataRemovalItemStatus::Failed);
    QVERIFY(fixture.cvRepository_.findById(imported.document_.id_).has_value());
    QVERIFY(QFileInfo::exists(imported.completedFilePath_));
    QVERIFY(!QFileInfo::exists(imported.completedFilePath_ + QStringLiteral(".delete")));
}

void DataRemovalServiceTest::recoversReferencedAndCommittedTombstones()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto imported = importCv(fixture, QStringLiteral("recover.pdf"));
    QCOMPARE(imported.disposition_, CvImportDisposition::Inserted);

    auto removal = fixture.fileStore_.prepareRemoval(imported.document_);
    QVERIFY2(removal.succeeded(), qPrintable(removal.message_));
    QVERIFY(!QFileInfo::exists(imported.completedFilePath_));
    QVERIFY(QFileInfo::exists(removal.preparation_->tombstoneFilePath_));
    auto report = fixture.fileStore_.reconcile(fixture.cvRepository_.findAll());
    QCOMPARE(report.restoredDeletionFileCount_, 1);
    QVERIFY(QFileInfo::exists(imported.completedFilePath_));

    removal.preparation_.reset();
    removal = fixture.fileStore_.prepareRemoval(imported.document_);
    QVERIFY(removal.succeeded());
    QVERIFY(fixture.cvRepository_.removeUnlinked(imported.document_.id_));
    removal.preparation_->databaseCommitted_ = true;
    report = fixture.fileStore_.reconcile(fixture.cvRepository_.findAll());
    QCOMPARE(report.removedDeletionFileCount_, 1);
    QVERIFY(!QFileInfo::exists(imported.completedFilePath_));
    QVERIFY(!QFileInfo::exists(removal.preparation_->tombstoneFilePath_));
}

void DataRemovalServiceTest::rejectsSymlinkWhenSupported()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto externalPath = fixture.storage_.createFile(QStringLiteral("external.pdf"));
    const auto linkName = QStringLiteral("linked-file.pdf");
    const auto linkPath = QDir{fixture.storage_.paths().resumesDirectory()}.filePath(linkName);
    if (!QFile::link(externalPath, linkPath)
        || !QFileInfo{linkPath}.exists()
        || !QFileInfo{linkPath}.isSymLink()) {
        QSKIP("Creating a filesystem link is not available in this test environment.");
    }

    CvDocument linkedFile;
    linkedFile.id_ = QStringLiteral("symlink-cv");
    linkedFile.originalFileName_ = linkName;
    linkedFile.storedFileName_ = linkName;
    linkedFile.relativePath_ = QStringLiteral("Resumes/%1").arg(linkName);
    linkedFile.sha256_ = QStringLiteral("symlink-hash");
    linkedFile.sizeBytes_ = 1;
    linkedFile.createdAt_ = QDateTime::currentDateTimeUtc();
    linkedFile.updatedAt_ = linkedFile.createdAt_;
    fixture.cvRepository_.insert(linkedFile);

    auto service = removalService(fixture);
    const auto result = service.process(
        DataRemovalKind::RemoveActiveCvs,
        requestFor(linkedFile.id_),
        std::make_shared<CancellationState>());

    QCOMPARE(result.items_.first().status_, DataRemovalItemStatus::Failed);
    QVERIFY(fixture.cvRepository_.findById(linkedFile.id_).has_value());
    QVERIFY(QFileInfo::exists(externalPath));
}

void DataRemovalServiceTest::mutationGateSerializesAddAndRemovalWork()
{
    StorageMutationGate gate;
    QVERIFY(gate.reserveJobSave());
    QVERIFY(!gate.beginRemoval());
    gate.releaseJobSave();

    QVERIFY(gate.reserveCvImports(2));
    QVERIFY(!gate.beginRemoval());
    gate.releaseCvImports(1);
    QVERIFY(!gate.beginRemoval());
    gate.releaseCvImports(1);

    QVERIFY(gate.beginRemoval());
    QVERIFY(!gate.reserveJobSave());
    QVERIFY(!gate.reserveCvImports(1));
    gate.endRemoval();
    QVERIFY(gate.reserveJobSave());
    gate.releaseJobSave();
}

void DataRemovalServiceTest::workerShutdownCancelsActiveWorkAndClosesItsConnection()
{
    const auto initialConnectionCount = QSqlDatabase::connectionNames().size();
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());

    DataRemovalWorker worker{storage.paths().dataDirectory()};
    QSignalSpy completedSpy{&worker, &DataRemovalWorker::removalCompleted};
    worker.submit({
        1,
        DataRemovalKind::DeleteJobs,
        {{QStringLiteral("missing-job"), QStringLiteral("Missing job")}},
        std::make_shared<CancellationState>()});
    QTRY_COMPARE_WITH_TIMEOUT(completedSpy.count(), 1, 10000);
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount + 1);

    QVector<DataRemovalItemRequest> items;
    items.reserve(100);
    for (int index = 0; index < 100; ++index) {
        const auto id = QStringLiteral("missing-job-%1").arg(index);
        items.append({id, id});
    }
    const auto cancellation = std::make_shared<CancellationState>();
    worker.submit({2, DataRemovalKind::DeleteJobs, std::move(items), cancellation});
    worker.shutdown();

    QVERIFY(cancellation->isCancellationRequested());
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);
}

QTEST_GUILESS_MAIN(DataRemovalServiceTest)

#include "DataRemovalServiceTest.moc"

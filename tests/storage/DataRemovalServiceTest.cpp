#include "../support/AddJobTestFixture.hpp"

#include "common/CancellationState.hpp"
#include "maintenance/DataRemovalService.hpp"
#include "maintenance/DataRemovalWorker.hpp"
#include "maintenance/StorageMutationGate.hpp"
#include "storage/SqlTransaction.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QThread>
#include <QUrl>
#include <QtTest/QtTest>

#include <mutex>

#include <memory>
#include <concepts>
#include <type_traits>
#include <utility>

namespace {

template<DataRemovalKind Kind>
concept HasTypedRemovalApi = requires(
    const DataRemovalService& service,
    DataRemovalExecutor& executor,
    DataRemovalWorker& worker,
    const QVector<DataRemovalItemRequest>& items,
    const std::shared_ptr<CancellationState>& cancellation)
{
    typename DataRemovalRequest<Kind>;
    typename DataRemovalBatchOutcome<Kind>;
    requires DataRemovalRequest<Kind>::kind_ == Kind;
    requires DataRemovalBatchOutcome<Kind>::kind_ == Kind;
    requires SingleActiveWorkerRequestFor<DataRemovalRequest<Kind>, DataRemovalExecutor>;
    { service.process<Kind>(items, cancellation) } -> std::same_as<DataRemovalBatchResult>;
    { executor.process(DataRemovalRequest<Kind>{}) } -> std::same_as<DataRemovalBatchOutcome<Kind>>;
    { worker.submit(DataRemovalRequest<Kind>{}) } -> std::same_as<void>;
};

static_assert(HasTypedRemovalApi<DataRemovalKind::DeleteJobs>);
static_assert(HasTypedRemovalApi<DataRemovalKind::RemoveActiveCvs>);
static_assert(HasTypedRemovalApi<DataRemovalKind::RestoreArchivedCvs>);
static_assert(HasTypedRemovalApi<DataRemovalKind::DeleteArchivedCvs>);
static_assert(!HasTypedRemovalApi<static_cast<DataRemovalKind>(-1)>);
static_assert(!HasTypedRemovalApi<static_cast<DataRemovalKind>(4)>);
static_assert(!std::is_member_object_pointer_v<decltype(&DeleteJobsOutcome::kind_)>);

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
    const std::unique_lock lock{fixture.cvLock_.getMutex()};
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

template<DataRemovalKind Kind>
void verifyCorrelation(
    const QVariant& value,
    quint64 operationId,
    const std::shared_ptr<CancellationState>& cancellation)
{
    using Outcome = DataRemovalBatchOutcome<Kind>;
    QCOMPARE(value.metaType(), QMetaType::fromType<Outcome>());
    const auto outcome = qvariant_cast<Outcome>(value);
    QCOMPARE(outcome.operationId_, operationId);
    QCOMPARE(outcome.cancellation_, cancellation);
}

template<DataRemovalKind Kind, typename Signal>
void verifyWorkerBoundaryOutcomes(Signal signal)
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    const auto initialConnections = QSqlDatabase::connectionNames().size();
    DataRemovalWorker worker{storage.paths().dataDirectory()};
    QSignalSpy completed{&worker, signal};
    QVERIFY(completed.isValid());
    using Outcome = DataRemovalBatchOutcome<Kind>;

    const auto emptyToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<Kind>{1, {}, emptyToken});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 10000);
    verifyCorrelation<Kind>(completed.at(0).at(0), 1, emptyToken);
    const auto empty = qvariant_cast<Outcome>(completed.at(0).at(0));
    QVERIFY(empty.result_.items_.isEmpty());
    QVERIFY(!empty.result_.cancelled_);

    const auto cancelledToken = std::make_shared<CancellationState>();
    cancelledToken->requestCancellation();
    worker.submit(DataRemovalRequest<Kind>{2, requestFor(QStringLiteral("missing")), cancelledToken});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 2, 10000);
    verifyCorrelation<Kind>(completed.at(1).at(0), 2, cancelledToken);
    const auto cancelled = qvariant_cast<Outcome>(completed.at(1).at(0));
    QVERIFY(cancelled.result_.cancelled_);
    QVERIFY(cancelled.result_.items_.isEmpty());

    const auto failedToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<Kind>{3, requestFor(QStringLiteral("missing")), failedToken});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 3, 10000);
    verifyCorrelation<Kind>(completed.at(2).at(0), 3, failedToken);
    const auto failed = qvariant_cast<Outcome>(completed.at(2).at(0));
    QCOMPARE(failed.result_.items_.size(), 1);
    QCOMPARE(failed.result_.items_.first().status_, DataRemovalItemStatus::Failed);
    QVERIFY(!failed.result_.items_.first().message_.isEmpty());

    worker.shutdown();
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnections);
    const auto rejectedToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<Kind>{4, requestFor(QStringLiteral("rejected")), rejectedToken});
    QCOMPARE(completed.count(), 3);
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 4, 10000);
    verifyCorrelation<Kind>(completed.at(3).at(0), 4, rejectedToken);
    const auto rejected = qvariant_cast<Outcome>(completed.at(3).at(0));
    QCOMPARE(rejected.result_.items_.size(), 1);
    QCOMPARE(rejected.result_.items_.first().status_, DataRemovalItemStatus::Failed);
    QCOMPARE(rejected.result_.items_.first().message_, QStringLiteral("The deletion worker is shutting down."));

    // A file cannot serve as the worker's data directory: context creation fails.
    const auto invalidDirectory = storage.createFile(QStringLiteral("not-a-directory.pdf"));
    QVERIFY(!invalidDirectory.isEmpty());
    DataRemovalWorker unavailable{invalidDirectory};
    QSignalSpy unavailableSpy{&unavailable, signal};
    const auto unavailableToken = std::make_shared<CancellationState>();
    unavailable.submit(DataRemovalRequest<Kind>{5, requestFor(QStringLiteral("unavailable")), unavailableToken});
    QTRY_COMPARE_WITH_TIMEOUT(unavailableSpy.count(), 1, 10000);
    verifyCorrelation<Kind>(unavailableSpy.first().at(0), 5, unavailableToken);
    const auto unavailableOutcome = qvariant_cast<Outcome>(unavailableSpy.first().at(0));
    QCOMPARE(unavailableOutcome.result_.items_.size(), 1);
    QCOMPARE(unavailableOutcome.result_.items_.first().status_, DataRemovalItemStatus::Failed);
    QVERIFY(!unavailableOutcome.result_.items_.first().message_.isEmpty());
    unavailable.shutdown();
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnections);
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
    void workerRoutesAllKindsAndKeepsCorrelation();
    void workerBoundaryOutcomesPreserveTypes();
    void busyRejectionsDoNotCompleteAcceptedWork();
    void workerCompletionCanSubmitNextBatch();
    void workerRetriesContextInitializationAfterFailure();
    void workerAcceptsNullCancellationToken();
    void workerReusesContextAndSuppressesPendingOutcomesAfterShutdown();
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
    const auto result = service.process<DataRemovalKind::DeleteJobs>(
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
    const auto result = service.process<DataRemovalKind::RemoveActiveCvs>(
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
    const auto result = service.process<DataRemovalKind::RemoveActiveCvs>(
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
    const auto result = service.process<DataRemovalKind::DeleteArchivedCvs>(
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
    const auto result = service.process<DataRemovalKind::RestoreArchivedCvs>(
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
    const auto result = service.process<DataRemovalKind::DeleteJobs>(
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
    const auto result = service.process<DataRemovalKind::RemoveActiveCvs>(
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
    const auto result = service.process<DataRemovalKind::RemoveActiveCvs>(
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
    const auto result = service.process<DataRemovalKind::RemoveActiveCvs>(
        requestFor(linkedFile.id_),
        std::make_shared<CancellationState>());

    QCOMPARE(result.items_.first().status_, DataRemovalItemStatus::Failed);
    QVERIFY(fixture.cvRepository_.findById(linkedFile.id_).has_value());
    QVERIFY(QFileInfo::exists(externalPath));
}

void DataRemovalServiceTest::mutationGateSerializesAddAndRemovalWork()
{
    StorageMutationGate gate;
    QVERIFY(gate.reserveImportOrUpdate());
    QVERIFY(!gate.beginRemoval());
    gate.releaseImportOrUpdate();

    QVERIFY(gate.reserveImportOrUpdate(2));
    QVERIFY(!gate.beginRemoval());
    gate.releaseImportOrUpdate(1);
    QVERIFY(!gate.beginRemoval());
    gate.releaseImportOrUpdate(1);

    QVERIFY(gate.beginRemoval());
    QVERIFY(!gate.reserveImportOrUpdate());
    QVERIFY(!gate.reserveImportOrUpdate(1));
    gate.endRemoval();
    QVERIFY(gate.reserveImportOrUpdate());
    gate.releaseImportOrUpdate();
}

void DataRemovalServiceTest::workerShutdownCancelsActiveWorkAndClosesItsConnection()
{
    const auto initialConnectionCount = QSqlDatabase::connectionNames().size();
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());

    DataRemovalWorker worker{storage.paths().dataDirectory()};
    QSignalSpy completedSpy{&worker, &DataRemovalWorker::jobsDeletionCompleted};
    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{
        1,
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
    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{2, std::move(items), cancellation});
    worker.shutdown();

    QVERIFY(cancellation->isCancellationRequested());
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);
}

void DataRemovalServiceTest::workerRoutesAllKindsAndKeepsCorrelation()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto created = fixture.service_.create(
        testsupport::validJobDraft(), QUrl::fromLocalFile(fixture.storage_.createFile()));
    QVERIFY(created.success_);
    const auto archived = importCv(fixture, QStringLiteral("archived.pdf"));
    QCOMPARE(archived.disposition_, CvImportDisposition::Inserted);
    QVERIFY(fixture.cvRepository_.updateArchived(archived.document_.id_, true).has_value());

    DataRemovalWorker worker{fixture.storage_.paths().dataDirectory()};
    QSignalSpy jobs{&worker, &DataRemovalWorker::jobsDeletionCompleted};
    QSignalSpy removed{&worker, &DataRemovalWorker::activeCvsRemovalCompleted};
    QSignalSpy restored{&worker, &DataRemovalWorker::archivedCvsRestorationCompleted};
    QSignalSpy deleted{&worker, &DataRemovalWorker::archivedCvsDeletionCompleted};
    QVERIFY(jobs.isValid() && removed.isValid() && restored.isValid() && deleted.isValid());

    const auto removeToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<DataRemovalKind::RemoveActiveCvs>{
        11, requestFor(created.cvDocument_.id_), removeToken});
    QTRY_COMPARE_WITH_TIMEOUT(removed.count(), 1, 10000);
    verifyCorrelation<DataRemovalKind::RemoveActiveCvs>(removed.first().at(0), 11, removeToken);
    const auto removeOutcome = qvariant_cast<RemoveActiveCvsOutcome>(removed.first().at(0));
    QCOMPARE(removeOutcome.result_.items_.size(), 1);
    QCOMPARE(removeOutcome.result_.items_.first().status_, DataRemovalItemStatus::Archived);
    QVERIFY(fixture.cvRepository_.findById(created.cvDocument_.id_)->archivedAt_.isValid());
    QCOMPARE(jobs.count() + restored.count() + deleted.count(), 0);

    const auto restoreToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<DataRemovalKind::RestoreArchivedCvs>{
        12, requestFor(created.cvDocument_.id_), restoreToken});
    QTRY_COMPARE_WITH_TIMEOUT(restored.count(), 1, 10000);
    verifyCorrelation<DataRemovalKind::RestoreArchivedCvs>(restored.first().at(0), 12, restoreToken);
    const auto restoreOutcome = qvariant_cast<RestoreArchivedCvsOutcome>(restored.first().at(0));
    QCOMPARE(restoreOutcome.result_.items_.size(), 1);
    QCOMPARE(restoreOutcome.result_.items_.first().status_, DataRemovalItemStatus::Restored);
    QVERIFY(!fixture.cvRepository_.findById(created.cvDocument_.id_)->archivedAt_.isValid());

    const auto jobToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{
        13,
        {{QStringLiteral("missing-job"), QStringLiteral("Missing")},
         {created.application_.id_, created.application_.jobTitle_}},
        jobToken});
    QTRY_COMPARE_WITH_TIMEOUT(jobs.count(), 1, 10000);
    verifyCorrelation<DataRemovalKind::DeleteJobs>(jobs.first().at(0), 13, jobToken);
    const auto jobOutcome = qvariant_cast<DeleteJobsOutcome>(jobs.first().at(0));
    QCOMPARE(jobOutcome.result_.items_.size(), 2);
    QCOMPARE(jobOutcome.result_.items_.at(0).status_, DataRemovalItemStatus::Failed);
    QCOMPARE(jobOutcome.result_.items_.at(1).status_, DataRemovalItemStatus::Deleted);
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QVERIFY(fixture.cvRepository_.findById(created.cvDocument_.id_).has_value());
    QCOMPARE(fixture.companyRepository_.findAll().size(), 1);

    const auto deleteToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteArchivedCvs>{
        14, requestFor(archived.document_.id_), deleteToken});
    QTRY_COMPARE_WITH_TIMEOUT(deleted.count(), 1, 10000);
    verifyCorrelation<DataRemovalKind::DeleteArchivedCvs>(deleted.first().at(0), 14, deleteToken);
    const auto deleteOutcome = qvariant_cast<DeleteArchivedCvsOutcome>(deleted.first().at(0));
    QCOMPARE(deleteOutcome.result_.items_.size(), 1);
    QCOMPARE(deleteOutcome.result_.items_.first().status_, DataRemovalItemStatus::Deleted);
    QVERIFY(!fixture.cvRepository_.findById(archived.document_.id_).has_value());
    QVERIFY(!QFileInfo::exists(archived.completedFilePath_));
    QCOMPARE(jobs.count(), 1);
    QCOMPARE(removed.count(), 1);
    QCOMPARE(restored.count(), 1);
}

void DataRemovalServiceTest::workerBoundaryOutcomesPreserveTypes()
{
    verifyWorkerBoundaryOutcomes<DataRemovalKind::DeleteJobs>(
        &DataRemovalWorker::jobsDeletionCompleted);
    verifyWorkerBoundaryOutcomes<DataRemovalKind::RemoveActiveCvs>(
        &DataRemovalWorker::activeCvsRemovalCompleted);
    verifyWorkerBoundaryOutcomes<DataRemovalKind::RestoreArchivedCvs>(
        &DataRemovalWorker::archivedCvsRestorationCompleted);
    verifyWorkerBoundaryOutcomes<DataRemovalKind::DeleteArchivedCvs>(
        &DataRemovalWorker::archivedCvsDeletionCompleted);
}

void DataRemovalServiceTest::busyRejectionsDoNotCompleteAcceptedWork()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto created = fixture.service_.create(
        testsupport::validJobDraft(), QUrl::fromLocalFile(fixture.storage_.createFile()));
    QVERIFY(created.success_);
    DataRemovalWorker worker{fixture.storage_.paths().dataDirectory()};
    QSignalSpy jobs{&worker, &DataRemovalWorker::jobsDeletionCompleted};
    QSignalSpy removed{&worker, &DataRemovalWorker::activeCvsRemovalCompleted};
    QSignalSpy restored{&worker, &DataRemovalWorker::archivedCvsRestorationCompleted};
    QSignalSpy deleted{&worker, &DataRemovalWorker::archivedCvsDeletionCompleted};

    // Open the worker's private connection before holding the database write lock.
    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{
        1, {}, std::make_shared<CancellationState>()});
    QTRY_COMPARE_WITH_TIMEOUT(jobs.count(), 1, 10000);
    SqlTransaction heldWrite{fixture.database_.connection(), QStringLiteral("Hold removal write")};
    QSqlQuery write{fixture.database_.connection()};
    QVERIFY(write.exec(QStringLiteral("UPDATE jobs SET notes = 'held write'")));
    write.finish();
    const auto acceptedToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{
        2, requestFor(created.application_.id_), acceptedToken});

    quint64 rejectedId = 10;
    const auto reject = [&]<DataRemovalKind Kind>(QSignalSpy& spy) {
        const auto previousCount = spy.count();
        const auto token = std::make_shared<CancellationState>();
        const auto id = ++rejectedId;
        worker.submit(DataRemovalRequest<Kind>{id, requestFor(QStringLiteral("rejected")), token});
        QCOMPARE(spy.count(), previousCount);
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), previousCount + 1, 2000);
        verifyCorrelation<Kind>(spy.last().at(0), id, token);
        const auto outcome = qvariant_cast<DataRemovalBatchOutcome<Kind>>(spy.last().at(0));
        QCOMPARE(outcome.result_.items_.size(), 1);
        QCOMPARE(outcome.result_.items_.first().status_, DataRemovalItemStatus::Failed);
        QCOMPARE(outcome.result_.items_.first().message_, QStringLiteral("Another deletion batch is already active."));
        QVERIFY(!acceptedToken->isCancellationRequested());
    };
    reject.template operator()<DataRemovalKind::DeleteJobs>(jobs);
    reject.template operator()<DataRemovalKind::RemoveActiveCvs>(removed);
    reject.template operator()<DataRemovalKind::RestoreArchivedCvs>(restored);
    reject.template operator()<DataRemovalKind::DeleteArchivedCvs>(deleted);
    QCOMPARE(jobs.count(), 2);
    heldWrite.commit();

    QTRY_COMPARE_WITH_TIMEOUT(jobs.count(), 3, 10000);
    verifyCorrelation<DataRemovalKind::DeleteJobs>(jobs.last().at(0), 2, acceptedToken);
    const auto accepted = qvariant_cast<DeleteJobsOutcome>(jobs.last().at(0));
    QCOMPARE(accepted.result_.items_.size(), 1);
    QCOMPARE(accepted.result_.items_.first().status_, DataRemovalItemStatus::Deleted);
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
}

void DataRemovalServiceTest::workerCompletionCanSubmitNextBatch()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto created = fixture.service_.create(
        testsupport::validJobDraft(), QUrl::fromLocalFile(fixture.storage_.createFile()));
    QVERIFY(created.success_);

    DataRemovalWorker worker{fixture.storage_.paths().dataDirectory()};
    QSignalSpy jobs{&worker, &DataRemovalWorker::jobsDeletionCompleted};
    QSignalSpy removed{&worker, &DataRemovalWorker::activeCvsRemovalCompleted};
    const auto jobToken = std::make_shared<CancellationState>();
    const auto cvToken = std::make_shared<CancellationState>();
    bool completedOnFacadeThread = false;
    QObject::connect(
        &worker,
        &DataRemovalWorker::jobsDeletionCompleted,
        &worker,
        [&](const DeleteJobsOutcome& outcome) {
            if (outcome.operationId_ == 1) {
                completedOnFacadeThread = QThread::currentThread() == worker.thread();
                worker.submit(DataRemovalRequest<DataRemovalKind::RemoveActiveCvs>{
                    2, requestFor(created.cvDocument_.id_), cvToken});
            }
        });

    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{
        1, requestFor(created.application_.id_), jobToken});
    QTRY_COMPARE_WITH_TIMEOUT(removed.count(), 1, 10000);
    QCOMPARE(jobs.count(), 1);
    QVERIFY(completedOnFacadeThread);
    verifyCorrelation<DataRemovalKind::DeleteJobs>(jobs.first().at(0), 1, jobToken);
    verifyCorrelation<DataRemovalKind::RemoveActiveCvs>(removed.first().at(0), 2, cvToken);
    const auto jobOutcome = qvariant_cast<DeleteJobsOutcome>(jobs.first().at(0));
    const auto cvOutcome = qvariant_cast<RemoveActiveCvsOutcome>(removed.first().at(0));
    QCOMPARE(jobOutcome.result_.items_.size(), 1);
    QCOMPARE(cvOutcome.result_.items_.size(), 1);
    QCOMPARE(jobOutcome.result_.items_.first().status_, DataRemovalItemStatus::Deleted);
    QCOMPARE(cvOutcome.result_.items_.first().status_, DataRemovalItemStatus::Deleted);
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QVERIFY(!fixture.cvRepository_.findById(created.cvDocument_.id_).has_value());
}

void DataRemovalServiceTest::workerRetriesContextInitializationAfterFailure()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    const auto initialConnectionCount = QSqlDatabase::connectionNames().size();
    const auto dataDirectory = storage.createFile(QStringLiteral("worker-data"));
    QVERIFY(!dataDirectory.isEmpty());
    DataRemovalWorker worker{dataDirectory};
    QSignalSpy completed{&worker, &DataRemovalWorker::jobsDeletionCompleted};

    const auto failedToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{
        1, requestFor(QStringLiteral("missing")), failedToken});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 10000);
    verifyCorrelation<DataRemovalKind::DeleteJobs>(completed.first().at(0), 1, failedToken);
    const auto failed = qvariant_cast<DeleteJobsOutcome>(completed.first().at(0));
    QCOMPARE(failed.result_.items_.size(), 1);
    QCOMPARE(failed.result_.items_.first().status_, DataRemovalItemStatus::Failed);
    QVERIFY(!failed.result_.items_.first().message_.isEmpty());
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);

    // Repair only the temporary fixture path, then retry with the same worker.
    QVERIFY(QFile::remove(dataDirectory));
    QVERIFY(QDir{}.mkpath(dataDirectory));
    const auto retryToken = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{2, {}, retryToken});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 2, 10000);
    verifyCorrelation<DataRemovalKind::DeleteJobs>(completed.last().at(0), 2, retryToken);
    const auto recovered = qvariant_cast<DeleteJobsOutcome>(completed.last().at(0));
    QVERIFY(recovered.result_.items_.isEmpty());
    QVERIFY(!recovered.result_.cancelled_);
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount + 1);

    worker.shutdown();
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);
}

void DataRemovalServiceTest::workerAcceptsNullCancellationToken()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto created = fixture.service_.create(
        testsupport::validJobDraft(), QUrl::fromLocalFile(fixture.storage_.createFile()));
    QVERIFY(created.success_);

    DataRemovalWorker worker{fixture.storage_.paths().dataDirectory()};
    QSignalSpy completed{&worker, &DataRemovalWorker::jobsDeletionCompleted};
    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{
        1, requestFor(created.application_.id_), nullptr});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 10000);
    verifyCorrelation<DataRemovalKind::DeleteJobs>(completed.first().at(0), 1, nullptr);
    const auto outcome = qvariant_cast<DeleteJobsOutcome>(completed.first().at(0));
    QVERIFY(!outcome.result_.cancelled_);
    QCOMPARE(outcome.result_.items_.size(), 1);
    QCOMPARE(outcome.result_.items_.first().status_, DataRemovalItemStatus::Deleted);
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
}

void DataRemovalServiceTest::workerReusesContextAndSuppressesPendingOutcomesAfterShutdown()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    const auto initialConnectionCount = QSqlDatabase::connectionNames().size();
    DataRemovalWorker worker{storage.paths().dataDirectory()};
    QSignalSpy completed{&worker, &DataRemovalWorker::jobsDeletionCompleted};
    QVERIFY(!worker.isRunning());
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);

    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{
        1, {}, std::make_shared<CancellationState>()});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 10000);
    QVERIFY(worker.isRunning());
    auto activeConnections = QSqlDatabase::connectionNames();
    activeConnections.sort();
    QCOMPARE(activeConnections.size(), initialConnectionCount + 1);

    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{
        2, {}, std::make_shared<CancellationState>()});
    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 2, 10000);
    auto reusedConnections = QSqlDatabase::connectionNames();
    reusedConnections.sort();
    QCOMPARE(reusedConnections, activeConnections);

    const auto cancellation = std::make_shared<CancellationState>();
    worker.submit(DataRemovalRequest<DataRemovalKind::DeleteJobs>{3, {}, cancellation});
    // Joining the worker leaves its accepted outcome queued on this thread.
    worker.shutdown();
    QVERIFY(cancellation->isCancellationRequested());
    QVERIFY(!worker.isRunning());
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);
    QCOMPARE(completed.count(), 2);
    QCoreApplication::sendPostedEvents(&worker, QEvent::MetaCall);
    QCOMPARE(completed.count(), 2);

    worker.shutdown();
    QCOMPARE(QSqlDatabase::connectionNames().size(), initialConnectionCount);
}

QTEST_GUILESS_MAIN(DataRemovalServiceTest)

#include "DataRemovalServiceTest.moc"

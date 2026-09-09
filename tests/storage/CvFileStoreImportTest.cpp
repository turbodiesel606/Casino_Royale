#include "cvs/CvImportService.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "cvs/CvMutationQueue.hpp"
#include "storage/SqlTransaction.hpp"
#include "common/CancellationState.hpp"

#include "../support/StorageTestFixtures.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QtTest/QtTest>

#include <atomic>
#include <latch>
#include <memory>
#include <thread>

class CvFileStoreImportTest final : public QObject
{
    Q_OBJECT

private slots:
    void buffersExactBytesWithoutManagedFiles();
    void stagesSnapshotAfterSourceChanges();
    void reportsStageFailureAfterSuccessfulBuffering();
    void cancelsBeforeBuffering();
    void cancelsBufferedImportBeforeStaging();
    void cancelsDuringStagingAndRemovesPartImmediately();
    void reportsUnavailablePreparation();
    void reconcilesStaleStagesAndQuarantinesOrphans();
    void reusesCvWithSameIdentity();
    void restoresArchivedExactDuplicate();

    void distinguishesSameContentWithDifferentNames();
    void distinguishesCaseOnlyFileNames();
    void distinguishesDifferentContentWithSameName();
    void importDispositionMessagesRemainExact();
};

void CvFileStoreImportTest::buffersExactBytesWithoutManagedFiles()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    CvManagedFileStore fileStore{storage.paths()};
    const QByteArray contents(3 * 1024 * 1024 + 137, 'x');
    const auto sourcePath = storage.createFile(QStringLiteral("streamed.pdf"), contents);
    const auto cancellation = std::make_shared<CancellationState>();

    const auto result = fileStore.prepare(QUrl::fromLocalFile(sourcePath), cancellation);

    QVERIFY2(result.succeeded(), qPrintable(result.message_));
    QCOMPARE(result.preparation_->sizeBytes_, static_cast<qint64>(contents.size()));
    QCOMPARE(
        result.preparation_->sha256_,
        QString::fromLatin1(QCryptographicHash::hash(contents, QCryptographicHash::Sha256).toHex()));
    QCOMPARE(result.preparation_->bytes_, contents);
    QCOMPARE(result.preparation_->originalFileName_, QStringLiteral("streamed.pdf"));
    QVERIFY(result.preparation_->storedFileName_.isEmpty());
    QVERIFY(result.preparation_->relativePath_.isEmpty());
    QVERIFY(result.preparation_->stagedFilePath_.isEmpty());
    QVERIFY(result.preparation_->finalFilePath_.isEmpty());
    QVERIFY(QDir{storage.paths().resumesDirectory()}.entryList(QDir::Files).isEmpty());
}

void CvFileStoreImportTest::stagesSnapshotAfterSourceChanges()
{
    testsupport::TemporaryStorageFixture storage;
    CvManagedFileStore fileStore{storage.paths()};
    const QByteArray contents(3 * 1024 * 1024 + 137, 's');
    const auto source = storage.createFile(QStringLiteral("snapshot.docx"), contents);
    const auto cancellation = std::make_shared<CancellationState>();
    const auto buffered = fileStore.prepare(QUrl::fromLocalFile(source), cancellation);
    QVERIFY(buffered.succeeded());
    QVERIFY(QFile::remove(source));

    const auto staged = fileStore.stageAndFinalize(*buffered.preparation_, cancellation);
    QVERIFY2(staged.succeeded(), qPrintable(staged.message_));
    QVERIFY(buffered.preparation_->stagedFilePath_.isEmpty());
    QVERIFY(!buffered.preparation_->storedFileName_.isEmpty());
    QFile finalFile{buffered.preparation_->finalFilePath_};
    QVERIFY(finalFile.open(QIODevice::ReadOnly));
    QCOMPARE(finalFile.readAll(), contents);
    QCOMPARE(QDir{storage.paths().resumesDirectory()}.entryList(QDir::Files).size(), 1);
}

void CvFileStoreImportTest::reportsStageFailureAfterSuccessfulBuffering()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    CvManagedFileStore fileStore{storage.paths()};
    const auto sourcePath = storage.createFile();
    QVERIFY(QDir{storage.paths().resumesDirectory()}.removeRecursively());
    QFile directoryBlocker{storage.paths().resumesDirectory()};
    QVERIFY(directoryBlocker.open(QIODevice::WriteOnly));
    directoryBlocker.close();
    const auto cancellation = std::make_shared<CancellationState>();

    const auto result = fileStore.prepare(QUrl::fromLocalFile(sourcePath), cancellation);

    QVERIFY2(result.succeeded(), qPrintable(result.message_));
    const auto staged = fileStore.stageAndFinalize(*result.preparation_, cancellation);
    QVERIFY(!staged.succeeded());
    QVERIFY(!staged.cancelled_);
    QVERIFY(staged.message_.contains(QStringLiteral("staged"), Qt::CaseInsensitive));
    QVERIFY(result.preparation_->stagedFilePath_.isEmpty());
    QVERIFY(result.preparation_->finalFilePath_.isEmpty());
}

void CvFileStoreImportTest::cancelsBeforeBuffering()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    CvManagedFileStore fileStore{storage.paths()};
    const auto sourcePath = storage.createFile(
        QStringLiteral("cancel.pdf"),
        QByteArray(2 * 1024 * 1024, 'c'));
    const auto cancellation = std::make_shared<CancellationState>();
    cancellation->requestCancellation();

    const auto result = fileStore.prepare(QUrl::fromLocalFile(sourcePath), cancellation);

    QVERIFY(!result.succeeded());
    QVERIFY(result.cancelled_);
    QCOMPARE(QDir{storage.paths().resumesDirectory()}.entryList(QDir::Files).size(), 0);
}

void CvFileStoreImportTest::cancelsBufferedImportBeforeStaging()
{
    testsupport::TemporaryDatabaseFixture fixture;
    CvRepository repository{fixture.database().connection()};
    CvManagedFileStore fileStore{fixture.storage().paths()};
    CvImportService importer{fileStore, repository};
    const auto cancellation = std::make_shared<CancellationState>();
    const auto buffered = importer.prepareDocument(
        QUrl::fromLocalFile(fixture.storage().createFile()), cancellation);
    QVERIFY(buffered.succeeded());
    cancellation->requestCancellation();

    const auto result = importer.importPreparedDocument(buffered.preparation_, cancellation);
    QVERIFY(!result.success_);
    QVERIFY(result.cancelled_);
    const auto staged = fileStore.stageAndFinalize(*buffered.preparation_, cancellation);
    QVERIFY(staged.cancelled_);
    QVERIFY(buffered.preparation_->storedFileName_.isEmpty());
    QVERIFY(buffered.preparation_->finalFilePath_.isEmpty());
    QVERIFY(repository.findAll().isEmpty());
    QVERIFY(QDir{fixture.storage().paths().resumesDirectory()}.entryList(QDir::Files).isEmpty());
}

void CvFileStoreImportTest::cancelsDuringStagingAndRemovesPartImmediately()
{
    testsupport::TemporaryStorageFixture storage;
    CvManagedFileStore fileStore{storage.paths()};
    const auto cancellation = std::make_shared<CancellationState>();
    const auto buffered = fileStore.prepare(QUrl::fromLocalFile(storage.createFile(
        QStringLiteral("large.pdf"), QByteArray(128 * 1024 * 1024, 'c'))), cancellation);
    QVERIFY(buffered.succeeded());

    std::atomic_bool sawPartialWrite = false;
    std::latch watcherReady{1};
    const auto directory = storage.paths().resumesDirectory();
    std::jthread cancelWriter{[&](std::stop_token stop) {
        watcherReady.count_down();
        while (!stop.stop_requested()) {
            const auto parts = QDir{directory}.entryInfoList({QStringLiteral("*.part")}, QDir::Files);
            if (!parts.isEmpty()) {
                // Inspect the open file: directory metadata can lag behind writes.
                QFile part{parts.first().absoluteFilePath()};
                if (part.open(QIODevice::ReadOnly)) {
                    const auto written = part.size();
                    part.close();
                    if (written > 0 && written < buffered.preparation_->sizeBytes_) {
                        sawPartialWrite = true;
                        cancellation->requestCancellation();
                        return;
                    }
                }
            }
            std::this_thread::yield();
        }
    }};
    watcherReady.wait();
    const auto staged = fileStore.stageAndFinalize(*buffered.preparation_, cancellation);
    cancelWriter.request_stop();
    cancelWriter.join();

    QVERIFY(sawPartialWrite.load());
    QVERIFY(!staged.succeeded());
    QVERIFY(staged.cancelled_);
    // Keep the preparation alive: cleanup must happen before it is destroyed.
    QVERIFY(buffered.preparation_->finalFilePath_.isEmpty());
    QVERIFY(buffered.preparation_->stagedFilePath_.isEmpty());
    QVERIFY(QDir{directory}.entryList(QDir::Files).isEmpty());
}

void CvFileStoreImportTest::reportsUnavailablePreparation()
{
    testsupport::TemporaryDatabaseFixture fixture;
    CvRepository repository{fixture.database().connection()};
    CvManagedFileStore fileStore{fixture.storage().paths()};
    CvImportService importer{fileStore, repository};
    const auto result = importer.importPreparedDocument({}, {});
    QVERIFY(!result.success_);
    QVERIFY(!result.cancelled_);
    QVERIFY(!result.message_.isEmpty());
}

void CvFileStoreImportTest::reconcilesStaleStagesAndQuarantinesOrphans()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    CvManagedFileStore fileStore{storage.paths()};
    const auto knownPath = storage.createFile(
        QStringLiteral("known.pdf"),
        QByteArrayLiteral("known"),
        storage.paths().resumesDirectory());
    const auto orphanPath = storage.createFile(
        QStringLiteral("orphan.docx"),
        QByteArrayLiteral("orphan"),
        storage.paths().resumesDirectory());
    const auto stagedPath = storage.createFile(
        QStringLiteral("interrupted.pdf.part"),
        QByteArrayLiteral("stage"),
        storage.paths().resumesDirectory());
    CvDocument knownDocument;
    knownDocument.storedFileName_ = QStringLiteral("known.pdf");

    const auto report = fileStore.reconcile({knownDocument});

    QCOMPARE(report.removedStagedFileCount_, 1);
    QCOMPARE(report.quarantinedFileNames_.size(), 1);
    QVERIFY(QFileInfo::exists(knownPath));
    QVERIFY(!QFileInfo::exists(orphanPath));
    QVERIFY(!QFileInfo::exists(stagedPath));
    QVERIFY(QFileInfo::exists(
        QDir{fileStore.quarantineDirectory()}.filePath(report.quarantinedFileNames_.first())));
}

void CvFileStoreImportTest::reusesCvWithSameIdentity()
{
    testsupport::TemporaryDatabaseFixture fixture;
    QVERIFY(fixture.isValid());
    CvRepository repository{fixture.database().connection()};
    CvManagedFileStore fileStore{fixture.storage().paths()};
    CvImportService importer{fileStore, repository};
    CvMutationQueue queue;
    auto lease = queue.acquire({});
    SqlTransaction transaction{fixture.database().connection(), QStringLiteral("CV resolver test")};
    const auto sourcePath = fixture.storage().createFile();
    const auto cancellation = std::make_shared<CancellationState>();

    auto firstPreparation = importer.prepareDocument(QUrl::fromLocalFile(sourcePath), cancellation);
    QVERIFY2(firstPreparation.succeeded(), qPrintable(firstPreparation.message_));
    const auto first = importer.importPreparedDocument(firstPreparation.preparation_, cancellation);

    auto secondPreparation = importer.prepareDocument(QUrl::fromLocalFile(sourcePath), cancellation);
    QVERIFY2(secondPreparation.succeeded(), qPrintable(secondPreparation.message_));
    const auto second = importer.importPreparedDocument(secondPreparation.preparation_, cancellation);
    QVERIFY(first.success_);
    QVERIFY(second.success_);
    QVERIFY(secondPreparation.preparation_->storedFileName_.isEmpty());
    QVERIFY(secondPreparation.preparation_->stagedFilePath_.isEmpty());
    QVERIFY(secondPreparation.preparation_->finalFilePath_.isEmpty());
    QCOMPARE(second.document_.storedFileName_, first.document_.storedFileName_);
    QCOMPARE(second.document_.updatedAt_, first.document_.updatedAt_);
    secondPreparation.preparation_.reset();

    QCOMPARE(first.disposition_, CvImportDisposition::Inserted);
    QCOMPARE(second.disposition_, CvImportDisposition::ExistingActive);
    QCOMPARE(first.document_.id_, second.document_.id_);
    QCOMPARE(repository.findAll().size(), 1);
    QCOMPARE(QDir{fixture.storage().paths().resumesDirectory()}.entryList(QDir::Files).size(), 1);
}

void CvFileStoreImportTest::restoresArchivedExactDuplicate()
{
    testsupport::TemporaryDatabaseFixture fixture;
    QVERIFY(fixture.isValid());
    CvRepository repository{fixture.database().connection()};
    CvManagedFileStore fileStore{fixture.storage().paths()};
    CvImportService importer{fileStore, repository};
    CvMutationQueue queue;
    auto lease = queue.acquire({});
    SqlTransaction transaction{fixture.database().connection(), QStringLiteral("CV resolver test")};
    const auto sourcePath = fixture.storage().createFile(QStringLiteral("restore.pdf"));
    const auto cancellation = std::make_shared<CancellationState>();

    auto preparation = importer.prepareDocument(QUrl::fromLocalFile(sourcePath), cancellation);
    QVERIFY(preparation.succeeded());
    const auto inserted = importer.importPreparedDocument(preparation.preparation_, cancellation);
    QCOMPARE(inserted.disposition_, CvImportDisposition::Inserted);
    QVERIFY(repository.updateArchived(inserted.document_.id_, true).has_value());

    preparation = importer.prepareDocument(QUrl::fromLocalFile(sourcePath), cancellation);
    QVERIFY(preparation.succeeded());
    const auto restored = importer.importPreparedDocument(
        preparation.preparation_,
        cancellation);
    QVERIFY(restored.success_);
    QVERIFY(preparation.preparation_->storedFileName_.isEmpty());
    QVERIFY(preparation.preparation_->stagedFilePath_.isEmpty());
    QVERIFY(preparation.preparation_->finalFilePath_.isEmpty());
    preparation.preparation_.reset();

    QCOMPARE(restored.disposition_, CvImportDisposition::RestoredArchived);
    QCOMPARE(restored.document_.id_, inserted.document_.id_);
    QVERIFY(!repository.findById(inserted.document_.id_)->archivedAt_.isValid());
    QCOMPARE(repository.findAll().size(), 1);
    QCOMPARE(QDir{fixture.storage().paths().resumesDirectory()}.entryList(QDir::Files).size(), 1);
}

void CvFileStoreImportTest::distinguishesSameContentWithDifferentNames()
{
    testsupport::TemporaryDatabaseFixture fixture;
    QVERIFY(fixture.isValid());
    CvRepository repository{fixture.database().connection()};
    CvManagedFileStore fileStore{fixture.storage().paths()};
    CvImportService importer{fileStore, repository};
    CvMutationQueue queue;
    auto lease = queue.acquire({});
    SqlTransaction transaction{fixture.database().connection(), QStringLiteral("CV resolver test")};
    const auto firstPath = fixture.storage().createFile(QStringLiteral("first.pdf"));
    const auto secondPath = fixture.storage().createFile(QStringLiteral("second.pdf"));
    const auto cancellation = std::make_shared<CancellationState>();

    const auto firstPreparation = importer.prepareDocument(QUrl::fromLocalFile(firstPath), cancellation);
    const auto secondPreparation = importer.prepareDocument(QUrl::fromLocalFile(secondPath), cancellation);
    QVERIFY(firstPreparation.succeeded());
    QVERIFY(secondPreparation.succeeded());
    const auto first = importer.importPreparedDocument(firstPreparation.preparation_, cancellation);
    const auto second = importer.importPreparedDocument(secondPreparation.preparation_, cancellation);

    QCOMPARE(first.disposition_, CvImportDisposition::Inserted);
    QCOMPARE(second.disposition_, CvImportDisposition::Inserted);
    QVERIFY(first.document_.id_ != second.document_.id_);
    QCOMPARE(repository.findAll().size(), 2);
}

void CvFileStoreImportTest::distinguishesCaseOnlyFileNames()
{
    testsupport::TemporaryDatabaseFixture fixture;
    QVERIFY(fixture.isValid());
    CvRepository repository{fixture.database().connection()};
    CvManagedFileStore fileStore{fixture.storage().paths()};
    CvImportService importer{fileStore, repository};
    CvMutationQueue queue;
    auto lease = queue.acquire({});
    SqlTransaction transaction{fixture.database().connection(), QStringLiteral("CV resolver test")};
    const auto upperDirectory = QDir{fixture.storage().rootPath()}
        .filePath(QStringLiteral("upper"));
    const auto lowerDirectory = QDir{fixture.storage().rootPath()}
        .filePath(QStringLiteral("lower"));
    QVERIFY(QDir{}.mkpath(upperDirectory));
    QVERIFY(QDir{}.mkpath(lowerDirectory));
    const auto contents = QByteArrayLiteral("%PDF-1.4 Same CV");
    const auto upperPath = fixture.storage().createFile(
        QStringLiteral("Resume.pdf"),
        contents,
        upperDirectory);
    const auto lowerPath = fixture.storage().createFile(
        QStringLiteral("resume.pdf"),
        contents,
        lowerDirectory);
    const auto cancellation = std::make_shared<CancellationState>();

    const auto upperPreparation = importer.prepareDocument(
        QUrl::fromLocalFile(upperPath),
        cancellation);
    const auto lowerPreparation = importer.prepareDocument(
        QUrl::fromLocalFile(lowerPath),
        cancellation);
    QVERIFY(upperPreparation.succeeded());
    QVERIFY(lowerPreparation.succeeded());
    const auto upper = importer.importPreparedDocument(upperPreparation.preparation_, cancellation);
    const auto lower = importer.importPreparedDocument(lowerPreparation.preparation_, cancellation);

    QCOMPARE(upper.disposition_, CvImportDisposition::Inserted);
    QCOMPARE(lower.disposition_, CvImportDisposition::Inserted);
    QVERIFY(upper.document_.id_ != lower.document_.id_);
    QCOMPARE(repository.findAll().size(), 2);
}

void CvFileStoreImportTest::distinguishesDifferentContentWithSameName()
{
    testsupport::TemporaryDatabaseFixture fixture;
    QVERIFY(fixture.isValid());
    CvRepository repository{fixture.database().connection()};
    CvManagedFileStore fileStore{fixture.storage().paths()};
    CvImportService importer{fileStore, repository};
    CvMutationQueue queue;
    auto lease = queue.acquire({});
    SqlTransaction transaction{fixture.database().connection(), QStringLiteral("CV resolver test")};
    const auto firstDirectory = QDir{fixture.storage().rootPath()}.filePath(QStringLiteral("first"));
    const auto secondDirectory = QDir{fixture.storage().rootPath()}.filePath(QStringLiteral("second"));
    QVERIFY(QDir{}.mkpath(firstDirectory));
    QVERIFY(QDir{}.mkpath(secondDirectory));
    const auto firstPath = fixture.storage().createFile(
        QStringLiteral("resume.pdf"),
        QByteArrayLiteral("%PDF-1.4 First CV"),
        firstDirectory);
    const auto secondPath = fixture.storage().createFile(
        QStringLiteral("resume.pdf"),
        QByteArrayLiteral("%PDF-1.4 Second CV"),
        secondDirectory);
    const auto cancellation = std::make_shared<CancellationState>();

    const auto firstPreparation = importer.prepareDocument(QUrl::fromLocalFile(firstPath), cancellation);
    const auto secondPreparation = importer.prepareDocument(QUrl::fromLocalFile(secondPath), cancellation);
    QVERIFY(firstPreparation.succeeded());
    QVERIFY(secondPreparation.succeeded());
    const auto first = importer.importPreparedDocument(firstPreparation.preparation_, cancellation);
    const auto second = importer.importPreparedDocument(secondPreparation.preparation_, cancellation);

    QCOMPARE(first.disposition_, CvImportDisposition::Inserted);
    QCOMPARE(second.disposition_, CvImportDisposition::Inserted);
    QVERIFY(first.document_.id_ != second.document_.id_);
    QCOMPARE(repository.findAll().size(), 2);
}

void CvFileStoreImportTest::importDispositionMessagesRemainExact()
{
    QCOMPARE(
        cvImportSuccessMessage(CvImportDisposition::Inserted),
        QStringLiteral("CV added successfully."));
    QCOMPARE(
        cvImportSuccessMessage(CvImportDisposition::RestoredArchived),
        QStringLiteral("The archived CV was restored to the library."));
    QCOMPARE(
        cvImportSuccessMessage(CvImportDisposition::ExistingActive),
        QStringLiteral("A CV with the same filename and SHA-256 already exists."));

}

QTEST_GUILESS_MAIN(CvFileStoreImportTest)

#include "CvFileStoreImportTest.moc"

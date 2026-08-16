#include "cvs/CvImportService.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "common/CancellationState.hpp"

#include "../support/StorageTestFixtures.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QtTest/QtTest>

#include <memory>

class CvFileStoreImportTest final : public QObject
{
    Q_OBJECT

private slots:
    void streamsHashAndStagesCopy();
    void reportsStagedCopyFailure();
    void cancelsPreparationAndCleansStage();
    void reconcilesStaleStagesAndQuarantinesOrphans();
    void reusesCvWithSameIdentity();
    void distinguishesSameContentWithDifferentNames();
    void distinguishesCaseOnlyFileNames();
    void distinguishesDifferentContentWithSameName();
};

void CvFileStoreImportTest::streamsHashAndStagesCopy()
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
    QVERIFY(QFileInfo::exists(result.preparation_->stagedFilePath_));
    QVERIFY(!QFileInfo::exists(result.preparation_->finalFilePath_));
}

void CvFileStoreImportTest::reportsStagedCopyFailure()
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

    QVERIFY(!result.succeeded());
    QVERIFY(!result.cancelled_);
    QVERIFY(result.message_.contains(QStringLiteral("staged"), Qt::CaseInsensitive));
}

void CvFileStoreImportTest::cancelsPreparationAndCleansStage()
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
    const auto sourcePath = fixture.storage().createFile();
    const auto cancellation = std::make_shared<CancellationState>();

    auto firstPreparation = importer.prepareDocument(QUrl::fromLocalFile(sourcePath), cancellation);
    QVERIFY2(firstPreparation.succeeded(), qPrintable(firstPreparation.message_));
    const auto first = importer.importPreparedDocument(firstPreparation.preparation_);

    auto secondPreparation = importer.prepareDocument(QUrl::fromLocalFile(sourcePath), cancellation);
    QVERIFY2(secondPreparation.succeeded(), qPrintable(secondPreparation.message_));
    const auto second = importer.importPreparedDocument(secondPreparation.preparation_);
    secondPreparation.preparation_.reset();

    QVERIFY(first.wasInserted_);
    QVERIFY(!second.wasInserted_);
    QCOMPARE(first.document_.id_, second.document_.id_);
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
    const auto firstPath = fixture.storage().createFile(QStringLiteral("first.pdf"));
    const auto secondPath = fixture.storage().createFile(QStringLiteral("second.pdf"));
    const auto cancellation = std::make_shared<CancellationState>();

    const auto firstPreparation = importer.prepareDocument(QUrl::fromLocalFile(firstPath), cancellation);
    const auto secondPreparation = importer.prepareDocument(QUrl::fromLocalFile(secondPath), cancellation);
    QVERIFY(firstPreparation.succeeded());
    QVERIFY(secondPreparation.succeeded());
    const auto first = importer.importPreparedDocument(firstPreparation.preparation_);
    const auto second = importer.importPreparedDocument(secondPreparation.preparation_);

    QVERIFY(first.wasInserted_);
    QVERIFY(second.wasInserted_);
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
    const auto upper = importer.importPreparedDocument(upperPreparation.preparation_);
    const auto lower = importer.importPreparedDocument(lowerPreparation.preparation_);

    QVERIFY(upper.wasInserted_);
    QVERIFY(lower.wasInserted_);
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
    const auto first = importer.importPreparedDocument(firstPreparation.preparation_);
    const auto second = importer.importPreparedDocument(secondPreparation.preparation_);

    QVERIFY(first.wasInserted_);
    QVERIFY(second.wasInserted_);
    QVERIFY(first.document_.id_ != second.document_.id_);
    QCOMPARE(repository.findAll().size(), 2);
}

QTEST_GUILESS_MAIN(CvFileStoreImportTest)

#include "CvFileStoreImportTest.moc"

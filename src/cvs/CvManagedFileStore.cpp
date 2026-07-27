#include "CvManagedFileStore.hpp"

#include "storage/StoragePaths.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

#include <stdexcept>

namespace {

constexpr qint64 copyBufferSize = 1024 * 1024;

bool isCancelled(const std::shared_ptr<std::atomic_bool>& cancellation)
{
    return cancellation != nullptr
        && cancellation->load(std::memory_order_relaxed);
}

QString managedStoredName(const QFileInfo& sourceInfo)
{
    auto baseName = sourceInfo.completeBaseName();
    baseName.replace(
        QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]+")),
        QStringLiteral("_"));
    if (baseName.isEmpty()) {
        baseName = QStringLiteral("Unnamed-resume");
    }

    return QStringLiteral("%1_%2_%3.%4")
        .arg(
            QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")),
            QUuid::createUuid().toString(QUuid::WithoutBraces),
            baseName,
            sourceInfo.suffix().toLower());
}

QString uniqueQuarantinePath(const QString& quarantineDirectory, const QString& fileName)
{
    const QDir directory{quarantineDirectory};
    auto candidate = directory.filePath(fileName);
    if (!QFileInfo::exists(candidate)) {
        return candidate;
    }

    const QFileInfo fileInfo{fileName};
    const auto suffix = fileInfo.completeSuffix();
    const auto uniqueName = suffix.isEmpty()
        ? QStringLiteral("%1_%2")
              .arg(fileInfo.completeBaseName(), QUuid::createUuid().toString(QUuid::WithoutBraces))
        : QStringLiteral("%1_%2.%3")
              .arg(
                  fileInfo.completeBaseName(),
                  QUuid::createUuid().toString(QUuid::WithoutBraces),
                  suffix);
    return directory.filePath(uniqueName);
}

}

CvManagedFilePreparation::~CvManagedFilePreparation()
{
    if (!stagedFilePath_.isEmpty()) {
        QFile::remove(stagedFilePath_);
    }
}

bool CvManagedFilePreparationResult::succeeded() const
{
    return preparation_ != nullptr && message_.isEmpty() && !cancelled_;
}

CvManagedFileStore::CvManagedFileStore(const StoragePaths& paths)
    : paths_(paths)
{
}

CvManagedFilePreparationResult CvManagedFileStore::prepare(
    const QUrl& sourceUrl,
    const std::shared_ptr<std::atomic_bool>& cancellation) const
{
    CvManagedFilePreparationResult result;
    if (isCancelled(cancellation)) {
        result.cancelled_ = true;
        result.message_ = QStringLiteral("Job creation was canceled.");
        return result;
    }
    if (!sourceUrl.isLocalFile()) {
        result.message_ = QStringLiteral("Select a local CV file.");
        return result;
    }

    const QFileInfo sourceInfo{sourceUrl.toLocalFile()};
    const auto extension = sourceInfo.suffix().toLower();
    if (!sourceInfo.exists() || !sourceInfo.isFile() || !sourceInfo.isReadable()) {
        result.message_ = QStringLiteral("The selected CV file is not readable.");
        return result;
    }
    if (extension != QStringLiteral("pdf")
        && extension != QStringLiteral("doc")
        && extension != QStringLiteral("docx")) {
        result.message_ = QStringLiteral("The CV must be a PDF, DOC, or DOCX file.");
        return result;
    }

    auto preparation = std::make_shared<CvManagedFilePreparation>();
    preparation->originalFileName_ = sourceInfo.fileName();
    preparation->storedFileName_ = managedStoredName(sourceInfo);
    preparation->relativePath_ = QDir::fromNativeSeparators(
        QStringLiteral("Resumes/%1").arg(preparation->storedFileName_));
    preparation->finalFilePath_ = QDir(paths_.resumesDirectory())
        .filePath(preparation->storedFileName_);
    preparation->stagedFilePath_ = preparation->finalFilePath_ + QStringLiteral(".part");
    result.preparation_ = preparation;

    QFile source{sourceInfo.absoluteFilePath()};
    if (!source.open(QIODevice::ReadOnly)) {
        result.message_ = QStringLiteral("The selected CV file could not be opened.");
        return result;
    }

    QFile staged{preparation->stagedFilePath_};
    if (!staged.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        result.message_ = QStringLiteral("The CV could not be staged in JobTracker storage.");
        return result;
    }

    QCryptographicHash hash{QCryptographicHash::Sha256};
    QByteArray buffer;
    buffer.resize(copyBufferSize);
    while (true) {
        if (isCancelled(cancellation)) {
            result.cancelled_ = true;
            result.message_ = QStringLiteral("Job creation was canceled.");
            return result;
        }

        const auto bytesRead = source.read(buffer.data(), buffer.size());
        if (bytesRead < 0) {
            result.message_ = QStringLiteral("The selected CV file could not be read.");
            return result;
        }
        if (bytesRead == 0) {
            break;
        }

        hash.addData(QByteArrayView{buffer.constData(), bytesRead});
        qint64 written = 0;
        while (written < bytesRead) {
            const auto bytesWritten = staged.write(buffer.constData() + written, bytesRead - written);
            if (bytesWritten <= 0) {
                result.message_ = QStringLiteral("The CV could not be staged in JobTracker storage.");
                return result;
            }
            written += bytesWritten;
        }
        preparation->sizeBytes_ += bytesRead;
    }

    if (!staged.flush()) {
        result.message_ = QStringLiteral("The CV could not be staged in JobTracker storage.");
        return result;
    }
    staged.close();
    source.close();

    if (isCancelled(cancellation)) {
        result.cancelled_ = true;
        result.message_ = QStringLiteral("Job creation was canceled.");
        return result;
    }

    preparation->sha256_ = QString::fromLatin1(hash.result().toHex());
    return result;
}

QString CvManagedFileStore::finalize(CvManagedFilePreparation& preparation) const
{
    if (preparation.stagedFilePath_.isEmpty()
        || !QFileInfo::exists(preparation.stagedFilePath_)
        || QFileInfo::exists(preparation.finalFilePath_)
        || !QFile::rename(preparation.stagedFilePath_, preparation.finalFilePath_)) {
        throw std::runtime_error("The staged CV could not be finalized in JobTracker storage.");
    }

    preparation.stagedFilePath_.clear();
    return preparation.finalFilePath_;
}

bool CvManagedFileStore::removeCompletedFile(const QString& completedFilePath) const
{
    return completedFilePath.isEmpty()
        || !QFileInfo::exists(completedFilePath)
        || QFile::remove(completedFilePath);
}

CvManagedFileRecoveryReport CvManagedFileStore::reconcile(
    const QVector<CvDocument>& documents) const
{
    CvManagedFileRecoveryReport report;
    QDirIterator stagedFiles{
        paths_.resumesDirectory(),
        {QStringLiteral("*.part")},
        QDir::Files | QDir::NoDotAndDotDot,
        QDirIterator::Subdirectories};
    while (stagedFiles.hasNext()) {
        const auto stagedPath = stagedFiles.next();
        if (!QFile::remove(stagedPath)) {
            throw std::runtime_error("A stale staged CV file could not be removed during startup recovery.");
        }
        ++report.removedStagedFileCount_;
    }

    const auto quarantinePath = quarantineDirectory();
    if (!QDir().mkpath(quarantinePath)) {
        throw std::runtime_error("The managed CV quarantine directory could not be created.");
    }

    QSet<QString> referencedStoredNames;
    referencedStoredNames.reserve(documents.size());
    for (const auto& document : documents) {
        referencedStoredNames.insert(document.storedFileName_);
    }

    const QDir resumesDirectory{paths_.resumesDirectory()};
    const auto completedFiles = resumesDirectory.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot,
        QDir::Name);
    for (const auto& fileInfo : completedFiles) {
        if (referencedStoredNames.contains(fileInfo.fileName())) {
            continue;
        }

        const auto destination = uniqueQuarantinePath(quarantinePath, fileInfo.fileName());
        if (!QFile::rename(fileInfo.absoluteFilePath(), destination)) {
            throw std::runtime_error("An orphaned managed CV file could not be moved to quarantine.");
        }
        report.quarantinedFileNames_.append(QFileInfo{destination}.fileName());
    }
    return report;
}

QString CvManagedFileStore::quarantineDirectory() const
{
    return QDir(paths_.resumesDirectory()).filePath(QStringLiteral("Quarantine"));
}

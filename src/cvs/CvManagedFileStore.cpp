#include "CvManagedFileStore.hpp"

#include "storage/StoragePaths.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSet>
#include <QUuid>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

	constexpr qint64 copyBufferSize = 1024 * 1024;

	bool isCancelled(const std::shared_ptr<CancellationState>& cancellation)
	{
		return cancellation != nullptr
			&& cancellation->isCancellationRequested();
	}

	QString managedStoredName(const QFileInfo& sourceInfo)
	{
		// Create file name with originalFileName_, creation time, sha256, extension
		auto baseName = sourceInfo.completeBaseName();
		baseName.replace(
			QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]+")),
			QStringLiteral("_"));
		if (baseName.isEmpty()) {
			baseName = QStringLiteral("Unnamed-resume");
		}

		return QStringLiteral("%1_%2_%3.%4")
			.arg(
				baseName,
				QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")),
				QUuid::createUuid().toString(QUuid::WithoutBraces),
				sourceInfo.suffix().toLower());
	}

	QString uniqueQuarantinePath(const QString& quarantineDirectory, const QString& fileName)
	{
		const QDir directory{ quarantineDirectory };
		auto candidate = directory.filePath(fileName);
		if (!QFileInfo::exists(candidate)) {
			return candidate;
		}

		const QFileInfo fileInfo{ fileName };
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
	// An unfinalized .part file is removed when preparation ownership ends.
	if (!stagedFilePath_.isEmpty())
		QFile::remove(stagedFilePath_);
}

CvManagedFileRemovalPreparation::~CvManagedFileRemovalPreparation()
{
	if (!databaseCommitted_
		&& !tombstoneFilePath_.isEmpty()
		&& QFileInfo::exists(tombstoneFilePath_)
		&& !QFileInfo::exists(originalFilePath_)) {
		QFile::rename(tombstoneFilePath_, originalFilePath_);
	}
}

bool CvManagedFilePreparationResult::succeeded() const
{
	return preparation_ != nullptr && message_.isEmpty() && !cancelled_;
}

bool CvManagedFileRemovalPreparationResult::succeeded() const
{
	return preparation_ != nullptr && message_.isEmpty();
}

CvManagedFileStore::CvManagedFileStore(const StoragePaths& paths)
	: paths_(paths)
	, pathResolver_{ paths }
{
}
bool CvManagedFileStageResult::succeeded() const
{
	return message_.isEmpty() && !cancelled_;
}

CvManagedFilePreparationResult CvManagedFileStore::prepare(
	const QUrl& sourceUrl,
	const std::shared_ptr<CancellationState>& cancellation) const
{
	// Reject cancellation before inspecting the source.
	if (isCancelled(cancellation))
		return { {}, QStringLiteral("CV import was canceled."), true };

	// Only local files can enter managed storage.
	if (!sourceUrl.isLocalFile())
		return { {}, QStringLiteral("Select a local CV file.") };

	// convert into a native filesystem path
	const QFileInfo sourceInfo{ sourceUrl.toLocalFile() };

	// extract the extension without dot and convert it to lowercase. 
	// E.g. PDF will become pdf, etc
	// See also check for extension below
	const auto extension = sourceInfo.suffix().toLower();

	// Source must exist, be a regular file, and be readable
	if (!sourceInfo.exists() || !sourceInfo.isFile())
		return { {}, QStringLiteral("The selected CV file is not readable.") };

	// Allow only supported CV document formats.
	/* Add the required file formats in the future. */
	if (extension != QStringLiteral("pdf")
		&& extension != QStringLiteral("doc")
		&& extension != QStringLiteral("docx")) {
		return { {}, QStringLiteral("The CV must be a PDF, DOC, or DOCX file.") };
	}
	// Open the user-selected CV for streaming reads.
	QFile source{ sourceInfo.absoluteFilePath() };

	if (!source.open(QIODevice::ReadOnly))
		return { {}, QStringLiteral("The selected CV file could not be opened.") };

	auto preparation = std::make_shared<CvManagedFilePreparation>();
	preparation->originalFileName_ = sourceInfo.fileName();
	auto& bytes = preparation->bytes_;

	QCryptographicHash hash{ QCryptographicHash::Sha256 };

	for (;;) {
		// Observe cooperative cancellation between read chunks.
		if (isCancelled(cancellation))
			return { {}, QStringLiteral("CV import was canceled."), true };

		const auto offset = bytes.size();

		// Prevent arithmetic overflow before extending buffer.
		if (offset > std::numeric_limits<qsizetype>::max() - copyBufferSize)
			return { {}, QStringLiteral("The selected CV file cannot fit in memory.") };

		bytes.resize(offset + copyBufferSize);

		/*
			Read 1 MB data at a time from source File.

			QIODevice::read() returns:

			case < 0: Read error;
			case > 0: Number of valid bytes placed into the buffer;
			case 0:	  No more data, end-of-file;
		*/

		// Read directly into the newly appended region.
		const auto bytesRead = source.read(bytes.data() + offset, copyBufferSize);

		if (bytesRead < 0)
			return { {}, QStringLiteral("The selected CV file could not be read.") };

		bytes.resize(offset + bytesRead);

		if (bytesRead == 0)
			break;
		// Dont try to move adding bytes outside the looop.
		// Adding data by chunks is more efficient than onetime record.
		hash.addData(QByteArrayView{ bytes.constData() + offset, bytesRead });
	}

	if (isCancelled(cancellation))
		return { {}, QStringLiteral("CV import was canceled."), true };

	// Identity and size describe the exact snapshot later written to storage.
	// No managed name or filesystem destination exists until an identity miss.
	preparation->sizeBytes_ = bytes.size();
	preparation->sha256_ = QString::fromLatin1(hash.result().toHex());
	return { std::move(preparation), {}, false };
}

CvManagedFileStageResult CvManagedFileStore::stageAndFinalize(
	CvManagedFilePreparation& preparation,
	// cancellation is stub, delete later
	const std::shared_ptr<CancellationState>& cancellation) const
{
	/*	Cancellation checks were turned off intentionally in this method!	*/

#if 0
	if (isCancelled(cancellation)) 
		return { QStringLiteral("CV import was canceled."), true };
#endif // 0

	// maybe this is unnecessary check...
	if (!preparation.stagedFilePath_.isEmpty() || !preparation.finalFilePath_.isEmpty())
		return { QStringLiteral("The prepared CV has already been staged.") };
	
	preparation.storedFileName_ = managedStoredName(QFileInfo{ preparation.originalFileName_ });
	preparation.relativePath_ = QStringLiteral("Resumes/%1").arg(preparation.storedFileName_);

	// obtain absolute filepath
	const auto finalPath = QDir{ paths_.resumesDirectory() }.filePath(preparation.storedFileName_);
	const auto stagedPath = finalPath + QStringLiteral(".part");

	QFile staged{ stagedPath };
	// create and open file
	if (!staged.open(QIODevice::WriteOnly | QIODevice::NewOnly)) 
		return { QStringLiteral("The CV could not be staged in JobTracker storage.") };
	
	preparation.stagedFilePath_ = stagedPath;

	// if we return early, delete unfinished .part file
	const auto cleanup = qScopeGuard([&] {
		staged.close();
		if (!preparation.stagedFilePath_.isEmpty()
			&& QFile::remove(preparation.stagedFilePath_)) {
			preparation.stagedFilePath_.clear();
		}
		});


	// file write logic starts
	
	qint64 written = 0;
	const auto& bytes = preparation.bytes_;
	// write from buffer to file
	while (written < bytes.size()) {

#if 0
		if (isCancelled(cancellation)) 
			return { QStringLiteral("CV import was canceled."), true };
#endif // 0

		/*
				********WARNING********
				QIODevice::write() is not guaranteed to write the specified number of bytes.
				So the checks are required!

				Example:
				bytesRead is 1000, but in the first iter write() writes only 400.
				600 are left, so we write them in the next iteration
			*/
		// successful write may be partial; advance only by its actual count.

		// Bound the next write to the remaining data and chunk size.

		const auto count = std::min(copyBufferSize, static_cast<qint64>(bytes.size()) - written);

		const auto bytesWritten = staged.write(bytes.constData() + written, count);
		if (bytesWritten <= 0) 
			return { QStringLiteral("The CV could not be staged in JobTracker storage.") };
		
		written += bytesWritten;
	}

	// Ensure buffered data reaches filesystem.
	if (!staged.flush()) 
		return { QStringLiteral("The CV could not be staged in JobTracker storage.") };
	
	staged.close();

#if 0
	if (isCancelled(cancellation)) 
		return { QStringLiteral("CV import was canceled."), true };
#endif // 0
	
	if (QFileInfo::exists(finalPath) || !QFile::rename(stagedPath, finalPath))
		return { QStringLiteral("The staged CV could not be finalized in JobTracker storage.") };
	

	// Final rename is the cancellation boundary. The caller must now commit
	// or roll back SQL and compensate this file before releasing its lease.
	preparation.finalFilePath_ = finalPath;
	preparation.stagedFilePath_.clear();
	return {};
}
bool CvManagedFileStore::removeCompletedFile(const QString& completedFilePath) const
{
	return completedFilePath.isEmpty()
		|| !QFileInfo::exists(completedFilePath)
		|| QFile::remove(completedFilePath);
}

CvManagedFileRemovalPreparationResult CvManagedFileStore::prepareRemoval(
	const CvDocument& document) const
{
	CvManagedFileRemovalPreparationResult result;
	const auto resolution = pathResolver_.resolve(document);
	if (!resolution.valid_) {
		result.message_ = resolution.message_;
		return result;
	}

	auto preparation = std::make_shared<CvManagedFileRemovalPreparation>();
	preparation->originalFilePath_ = resolution.absolutePath_;
	result.preparation_ = preparation;
	if (!resolution.exists_) {
		result.fileWasMissing_ = true;
		return result;
	}

	preparation->tombstoneFilePath_ = resolution.absolutePath_ + QStringLiteral(".delete");
	if (QFileInfo::exists(preparation->tombstoneFilePath_)
		|| !QFile::rename(preparation->originalFilePath_, preparation->tombstoneFilePath_)) {
		result.message_ = QStringLiteral("The managed CV file could not be prepared for deletion.");
	}
	return result;
}

bool CvManagedFileStore::finalizeRemoval(CvManagedFileRemovalPreparation& preparation) const
{
	preparation.databaseCommitted_ = true;
	if (preparation.tombstoneFilePath_.isEmpty()
		|| !QFileInfo::exists(preparation.tombstoneFilePath_)) {
		return true;
	}
	if (!QFile::remove(preparation.tombstoneFilePath_)) {
		return false;
	}
	preparation.tombstoneFilePath_.clear();
	return true;
}

CvManagedFileRecoveryReport CvManagedFileStore::reconcile(
	const QVector<CvDocument>& documents) const
{	// Reconciles managed CV files with persisted CV records during application startup

	CvManagedFileRecoveryReport report;
	QSet<QString> referencedStoredNames;
	for (const auto& document : documents) {
		referencedStoredNames.insert(document.storedFileName_);
	}

	const QDir resumesDirectory{ paths_.resumesDirectory() };
	const auto deletionFiles = resumesDirectory.entryInfoList(
		{ QStringLiteral("*.delete") },
		QDir::Files | QDir::NoDotAndDotDot,
		QDir::Name);
	for (const auto& fileInfo : deletionFiles) {
		auto storedFileName = fileInfo.fileName();
		storedFileName.chop(QStringLiteral(".delete").size());
		const auto originalPath = resumesDirectory.filePath(storedFileName);
		if (referencedStoredNames.contains(storedFileName)
			&& !QFileInfo::exists(originalPath)) {
			if (!QFile::rename(fileInfo.absoluteFilePath(), originalPath)) {
				throw std::runtime_error("A referenced managed CV deletion tombstone could not be restored.");
			}
			++report.restoredDeletionFileCount_;
		}
		else {
			if (!QFile::remove(fileInfo.absoluteFilePath())) {
				throw std::runtime_error("A managed CV deletion tombstone could not be removed.");
			}
			++report.removedDeletionFileCount_;
		}
	}

	QDirIterator stagedFiles{
		paths_.resumesDirectory(),
		{QStringLiteral("*.part")},
		QDir::Files | QDir::NoDotAndDotDot,
		QDirIterator::Subdirectories };

	// Remove stale temporary CV files left by interrupted preparations.
	while (stagedFiles.hasNext()) {
		const auto stagedPath = stagedFiles.next();

		if (!QFile::remove(stagedPath))
			throw std::runtime_error("A stale staged CV file could not be removed during startup recovery.");

		++report.removedStagedFileCount_;
	}
	// Create quarantine directory
	const auto quarantinePath = quarantineDirectory();
	if (!QDir().mkpath(quarantinePath))
		throw std::runtime_error("The managed CV quarantine directory could not be created.");

	// Get each file info from .../Resumes/
	const auto completedFiles = resumesDirectory.entryInfoList(
		QDir::Files | QDir::NoDotAndDotDot,
		QDir::Name);

	for (const auto& fileInfo : completedFiles) {
		if (referencedStoredNames.contains(fileInfo.fileName()))
			// check whether DB reference to file
			continue;

		// If we didn`t find name, it is orphaned file. 
		// It is on disk, but DB dont know anything about it.
		const auto destination = uniqueQuarantinePath(quarantinePath, fileInfo.fileName());

		// move from Resumes/ to Quarantine/
		if (!QFile::rename(fileInfo.absoluteFilePath(), destination))
			throw std::runtime_error("An orphaned managed CV file could not be moved to quarantine.");

		report.quarantinedFileNames_.append(QFileInfo{ destination }.fileName());
	}
	return report;
}

QString CvManagedFileStore::quarantineDirectory() const
{
	return QDir(paths_.resumesDirectory()).filePath(QStringLiteral("Quarantine"));
}

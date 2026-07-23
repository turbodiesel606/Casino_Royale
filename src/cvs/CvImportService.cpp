#include "CvImportService.hpp"

#include "CvRepository.hpp"
#include "storage/StoragePaths.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>
#include <stdexcept>
#include<iostream>

// Keep references to storage paths and the CV repository used during imports.
namespace {

	CvDocument buildCvDocument(const QFileInfo& sourceInfo, const QString& storedName, const QString& hash)
	{
		CvDocument document;

		document.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
		document.fileName_ = sourceInfo.fileName();
		document.originalFileName_ = sourceInfo.fileName();
		document.storedFileName_ = storedName;
		document.relativePath_ = QDir::fromNativeSeparators(QStringLiteral("Resumes/%1").arg(storedName));
		document.sha256_ = hash;
		document.sizeBytes_ = sourceInfo.size();
		document.title_ = sourceInfo.completeBaseName();
		document.category_ = QStringLiteral("General");
		document.categoryAccent_ = QStringLiteral("#1687ff");
		document.languageAccent_ = QStringLiteral("#65bf4c");
		document.fileSizeLabel_ = QStringLiteral("%1 KB").arg((document.sizeBytes_ + 1023) / 1024);
		document.createdAt_ = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
		document.updatedAt_ = document.createdAt_;
		document.lastModifiedLabel_ = QDate::currentDate().toString(QStringLiteral("MMM d, yyyy"));

		return document;
	}

}
CvImportService::CvImportService(const StoragePaths& paths, CvRepository& repository)
	: paths_(paths)
	, repository_(repository)
{
}

/*
change dublicate file logic later:
../Root/Requirements/File Storage.docx
*/

CvImportResult CvImportService::importDocument(const QUrl& sourceUrl) const
{   // Import a local CV file into managed storage and save its metadata in the database.
	if (!sourceUrl.isLocalFile()) // Only local files can be copied into JobTracker storage.
		throw std::runtime_error("Select a local CV file.");

	const QFileInfo sourceInfo(sourceUrl.toLocalFile()); // convert ugly extension to normal
	const auto extension = sourceInfo.suffix().toLower(); // find out extension

	if (!sourceInfo.exists() || !sourceInfo.isFile() || !sourceInfo.isReadable())
		throw std::runtime_error("The selected CV file is not readable.");

	if (extension != QStringLiteral("pdf")
		&& extension != QStringLiteral("doc")
		&& extension != QStringLiteral("docx")) {
		throw std::runtime_error("The CV must be a PDF, DOC, or DOCX file.");
	}

	// Hash the file contents so duplicate CVs can be detected by content, not filename.
	QFile source(sourceInfo.absoluteFilePath());
	if (!source.open(QIODevice::ReadOnly))
		throw std::runtime_error("The selected CV file could not be opened.");

	const auto hash = QString::fromLatin1(QCryptographicHash::hash(source.readAll(), QCryptographicHash::Sha256).toHex());
	source.close();

	// if there is a file with that hash, return it (Reuse)
	if (const auto existing = repository_.findBySha256(hash))
		return { *existing, {}, false };


	auto baseName = sourceInfo.completeBaseName(); // get name without extension (ex: .pdf)
	// Any character that is not a letter, number, underscore, or dash is replaced with _.
	baseName.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]+")), QStringLiteral("_"));

	if (baseName.isEmpty())
		baseName = QStringLiteral("Unnamed-resume");

	//Generate a unique stored filename
	const auto storedName = QStringLiteral("%1_%2_%3.%4")
		.arg(
			QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")),
			QUuid::createUuid().toString(QUuid::WithoutBraces),
			baseName,
			extension);

	// Copy to temporary file and rename it to initial name after successful copy
	const auto finalPath = QDir(paths_.resumesDirectory()).filePath(storedName);
	const auto temporaryPath = finalPath + QStringLiteral(".part");
	if (!QFile::copy(sourceInfo.absoluteFilePath(), temporaryPath) || !QFile::rename(temporaryPath, finalPath)) {
		QFile::remove(temporaryPath);
		QFile::remove(finalPath);
		throw std::runtime_error("The CV could not be copied into JobTracker storage.");
	}

	CvDocument document = buildCvDocument(sourceInfo, storedName, hash);

	try {
		repository_.insert(document);
	}
	catch (...) {
		QFile::remove(finalPath);
		throw;
	}
	return { document, finalPath, true };
}

#include "CvRepository.hpp"
#include "utils/Utils.hpp"
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <stdexcept>
#include <iostream>

namespace {

	CvDocument convertToCvDocument(const QSqlQuery& query)
	{   // Convert cvs table to CvDocument object
		CvDocument document;
		document.id_ = query.value(QStringLiteral("id")).toString();
		document.originalFileName_ = query.value(QStringLiteral("original_file_name")).toString();
		document.storedFileName_ = query.value(QStringLiteral("stored_file_name")).toString();
		document.fileName_ = document.originalFileName_; // This means the app wants the display filename to mirror the original uploaded filename.
		document.relativePath_ = query.value(QStringLiteral("relative_path")).toString();
		document.sha256_ = query.value(QStringLiteral("sha256")).toString();
		document.sizeBytes_ = query.value(QStringLiteral("size_bytes")).toLongLong();
		document.title_ = query.value(QStringLiteral("title")).toString();
		document.category_ = query.value(QStringLiteral("category")).toString();
		document.language_ = query.value(QStringLiteral("language")).toString();
		document.description_ = query.value(QStringLiteral("description")).toString();
		document.isFavorite_ = query.value(QStringLiteral("is_favorite")).toBool();
		document.createdAt_ = query.value(QStringLiteral("created_at")).toString();
		document.updatedAt_ = query.value(QStringLiteral("updated_at")).toString();

		// convert ISO timestamp into a local date label like Jul 15, 2026.
		document.lastModifiedLabel_ = QDateTime::fromString(document.updatedAt_, Qt::ISODate).toLocalTime().date().toString(QStringLiteral("MMM d, yyyy"));

		document.fileSizeLabel_ = QStringLiteral("%1 KB").arg((document.sizeBytes_ + 1023) / 1024);
		document.categoryAccent_ = QStringLiteral("#1687ff");
		document.languageAccent_ = QStringLiteral("#65bf4c");
		return document;
	}

	QString selectCvsWithLinkedJob()
	{
		/* Query:
		SELECT
		cvs.id,
		cvs.original_file_name,
		cvs.stored_file_name,
		cvs.relative_path,
		cvs.sha256,
		cvs.size_bytes,
		cvs.title,
		cvs.category,
		cvs.language,
		cvs.description,
		cvs.is_favorite,
		cvs.created_at,
		cvs.updated_at,
		jobs.id AS job_id
		FROM cvs
		LEFT JOIN jobs ON jobs.cv_id = cvs.id
		ORDER BY cvs.created_at DESC, jobs.created_at ASC
		*/
		return QStringLiteral(
			"SELECT "
			"    cvs.id, "
			"    cvs.original_file_name, "
			"    cvs.stored_file_name, "
			"    cvs.relative_path, "
			"    cvs.sha256, "
			"    cvs.size_bytes, "
			"    cvs.title, "
			"    cvs.category, "
			"    cvs.language, "
			"    cvs.description, "
			"    cvs.is_favorite, "
			"    cvs.created_at, "
			"    cvs.updated_at, "
			"    jobs.id AS job_id "
			"FROM cvs "
			"LEFT JOIN jobs ON jobs.cv_id = cvs.id "
			"ORDER BY cvs.created_at DESC, jobs.created_at ASC");
	}

	QString sqlText(const QString& value) { return value.isNull() ? QStringLiteral("") : value; }

}

CvRepository::CvRepository(QSqlDatabase& database)
	: database_{ database }
{
}

QVector<CvDocument> CvRepository::findAll() const
{
	QSqlQuery cvsRowCountQuery{ database_ };
	// find out row count
	if (!cvsRowCountQuery.exec(QStringLiteral("SELECT COUNT(*) FROM cvs")) || !cvsRowCountQuery.next())
		utils::throwQueryError(cvsRowCountQuery);

	const qsizetype cvsRowCount = static_cast<qsizetype>(cvsRowCountQuery.value(0).toLongLong());

	if (!cvsRowCount) return{};

	QSqlQuery cvsJoinedQuery(database_);
	if (!cvsJoinedQuery.exec(selectCvsWithLinkedJob()))
		utils::throwQueryError(cvsJoinedQuery);

	QVector<CvDocument> documents;
	QHash<QString, qsizetype> documentIndexes;
	documents.reserve(cvsRowCount);
	documentIndexes.reserve(cvsRowCount);

	while (cvsJoinedQuery.next()) {
		const QString cvId = cvsJoinedQuery.value(QStringLiteral("id")).toString();

		auto indexIt = documentIndexes.constFind(cvId); // get iter to CV ID`value
		if (indexIt == documentIndexes.constEnd()) { // if this CV wasn`t added, add it
			documents.append(convertToCvDocument(cvsJoinedQuery));

			const qsizetype index = documents.size() - 1;
			indexIt = documentIndexes.insert(cvId, index);
		}

		const QString job_id = cvsJoinedQuery.value(QStringLiteral("job_id")).toString();

		if (!job_id.isEmpty()) // add all jobs to current CV
			documents[*indexIt].linkedApplicationIds_.append(job_id);

	}
	return documents;
}
	
std::optional<CvDocument> CvRepository::findBySha256(const QString& sha256) const
{
	QSqlQuery query{ database_ };
	query.prepare(QStringLiteral("SELECT * FROM cvs WHERE sha256 = ?"));
	query.addBindValue(sha256);

	if (!query.exec())
		utils::throwQueryError(query);

	if (!query.next())
		return std::nullopt;

	return convertToCvDocument(query);
}

void CvRepository::insert(const CvDocument& document) const
{
	QSqlQuery query{ database_ };
	query.prepare(QStringLiteral(
		"INSERT INTO cvs (id, original_file_name, stored_file_name, relative_path, sha256,"
		" size_bytes, title, category, language, description, is_favorite, created_at, updated_at)"
		" VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
	query.addBindValue(document.id_);
	query.addBindValue(document.originalFileName_);
	query.addBindValue(document.storedFileName_);
	query.addBindValue(document.relativePath_);
	query.addBindValue(document.sha256_);
	query.addBindValue(document.sizeBytes_);
	query.addBindValue(document.title_.isNull() ? QString{} : document.title_);
	query.addBindValue(document.category_.isNull() ? QString{} : document.category_);
	query.addBindValue(document.language_.isNull() ? QString{} : document.language_);
	query.addBindValue(document.description_.isNull() ? QString{} : document.description_);
	query.addBindValue(document.isFavorite_);
	query.addBindValue(document.createdAt_);
	query.addBindValue(document.updatedAt_);

	if (!query.exec())
		utils::throwQueryError(query);

}

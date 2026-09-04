#include "CvRepository.hpp"

#include "storage/SqlQuery.hpp"

#include <QHash>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <utility>

namespace {

CvDocument convertToCvDocument(const QSqlQuery& query)
{
    CvDocument document;
    document.id_ = query.value(QStringLiteral("id")).toString();
    document.originalFileName_ = query.value(QStringLiteral("original_file_name")).toString();
    document.storedFileName_ = query.value(QStringLiteral("stored_file_name")).toString();
    document.relativePath_ = query.value(QStringLiteral("relative_path")).toString();
    document.sha256_ = query.value(QStringLiteral("sha256")).toString();
    document.sizeBytes_ = query.value(QStringLiteral("size_bytes")).toLongLong();
    document.title_ = query.value(QStringLiteral("title")).toString();
    document.category_ = query.value(QStringLiteral("category")).toString();
    document.language_ = query.value(QStringLiteral("language")).toString();
    document.description_ = query.value(QStringLiteral("description")).toString();
    document.isFavorite_ = query.value(QStringLiteral("is_favorite")).toBool();
    document.createdAt_ = storage::sql::readIsoDateTime(query, QStringLiteral("created_at"));
    document.updatedAt_ = storage::sql::readIsoDateTime(query, QStringLiteral("updated_at"));
    document.archivedAt_ = storage::sql::readIsoDateTime(query, QStringLiteral("archived_at"));
    return document;
}

const QString& joinedCvSelect()
{
    static const QString statement = QStringLiteral(
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
        "    cvs.archived_at, "
        "    jobs.id AS job_id "
        "FROM cvs "
        "LEFT JOIN jobs ON jobs.cv_id = cvs.id ");
    return statement;
}

QVector<CvDocument> readJoinedDocuments(QSqlQuery& query)
{
    QVector<CvDocument> documents;
    QHash<QString, qsizetype> documentIndexes;

    while (query.next()) {
        const auto cvId = query.value(QStringLiteral("id")).toString();
        auto indexIt = documentIndexes.constFind(cvId);
        if (indexIt == documentIndexes.constEnd()) {
            documents.append(convertToCvDocument(query));
            indexIt = documentIndexes.insert(cvId, documents.size() - 1);
        }

        const auto jobId = query.value(QStringLiteral("job_id")).toString();
        if (!jobId.isEmpty()) {
            documents[*indexIt].linkedApplicationIds_.append(jobId);
        }
    }
    return documents;
}

} // namespace

CvRepository::CvRepository(QSqlDatabase& database)
    : database_{database}
{
}

QVector<CvDocument> CvRepository::findAll() const
{
    QSqlQuery query{database_};
    query.prepare(
        joinedCvSelect()
        + QStringLiteral("ORDER BY cvs.created_at DESC, jobs.created_at ASC"));
    storage::sql::execute(query, QStringLiteral("load CVs and linked job applications"));
    return readJoinedDocuments(query);
}

std::optional<CvDocument> CvRepository::findById(const QString& cvId) const
{
    QSqlQuery query{database_};
    query.prepare(
        joinedCvSelect()
        + QStringLiteral("WHERE cvs.id = ? ORDER BY jobs.created_at ASC"));
    query.addBindValue(cvId);
    storage::sql::execute(query, QStringLiteral("find a CV by ID"));

    auto documents = readJoinedDocuments(query);
    if (documents.isEmpty()) {
        return std::nullopt;
    }
    return std::move(documents.front());
}

std::optional<CvDocument> CvRepository::findByIdentity(
    const QString& sha256,
    const QString& originalFileName) const
{
    QSqlQuery query{database_};
    query.prepare(QStringLiteral(
        "SELECT * FROM cvs WHERE sha256 = ? AND original_file_name = ?"));
    query.addBindValue(sha256);
    query.addBindValue(originalFileName);
    storage::sql::execute(query, QStringLiteral("find a CV by identity"));

    if (!query.next())
        return std::nullopt;

    return convertToCvDocument(query);
}

void CvRepository::insert(const CvDocument& document) const
{
    QSqlQuery query{database_};
    query.prepare(QStringLiteral(
        "INSERT INTO cvs (id, original_file_name, stored_file_name, relative_path, sha256,"
        " size_bytes, title, category, language, description, is_favorite, created_at, updated_at, archived_at)"
        " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(document.id_);
    query.addBindValue(document.originalFileName_);
    query.addBindValue(document.storedFileName_);
    query.addBindValue(document.relativePath_);
    query.addBindValue(document.sha256_);
    query.addBindValue(document.sizeBytes_);
    query.addBindValue(storage::sql::nonNullText(document.title_));
    query.addBindValue(storage::sql::nonNullText(document.category_));
    query.addBindValue(storage::sql::nonNullText(document.language_));
    query.addBindValue(storage::sql::nonNullText(document.description_));
    query.addBindValue(document.isFavorite_);
    query.addBindValue(document.createdAt_.toUTC().toString(Qt::ISODate));
    query.addBindValue(document.updatedAt_.toUTC().toString(Qt::ISODate));
    query.addBindValue(document.archivedAt_.isValid()
        ? QVariant{document.archivedAt_.toUTC().toString(Qt::ISODateWithMs)}
        : QVariant{});
    storage::sql::execute(query, QStringLiteral("insert a CV"));
}

std::optional<QDateTime> CvRepository::updateFavorite(
    const QString& cvId,
    bool isFavorite) const
{
    const auto updatedAt = QDateTime::currentDateTimeUtc();
    QSqlQuery query{database_};
    query.prepare(QStringLiteral(
        "UPDATE cvs SET is_favorite = ?, updated_at = ? WHERE id = ?"));
    query.addBindValue(isFavorite);
    query.addBindValue(updatedAt.toString(Qt::ISODateWithMs));
    query.addBindValue(cvId);
    storage::sql::execute(query, QStringLiteral("update a CV favorite"));

    return query.numRowsAffected() == 1
        ? std::optional<QDateTime>{updatedAt}
        : std::nullopt;
}

std::optional<QDateTime> CvRepository::updateArchived(
    const QString& cvId,
    bool archived) const
{
    const auto updatedAt = QDateTime::currentDateTimeUtc();
    QSqlQuery query{database_};
    query.prepare(QStringLiteral(
        "UPDATE cvs SET archived_at = ?, updated_at = ? WHERE id = ?"));
    query.addBindValue(archived
        ? QVariant{updatedAt.toString(Qt::ISODateWithMs)}
        : QVariant{});
    query.addBindValue(updatedAt.toString(Qt::ISODateWithMs));
    query.addBindValue(cvId);
    storage::sql::execute(query, QStringLiteral("update a CV archive state"));

    return query.numRowsAffected() == 1
        ? std::optional<QDateTime>{updatedAt}
        : std::nullopt;
}

bool CvRepository::removeUnlinked(const QString& cvId) const
{
    QSqlQuery query{database_};
    query.prepare(QStringLiteral(
        "DELETE FROM cvs WHERE id = ? "
        "AND NOT EXISTS (SELECT 1 FROM jobs WHERE jobs.cv_id = cvs.id)"));
    query.addBindValue(cvId);
    storage::sql::execute(query, QStringLiteral("delete an unlinked CV"));
    return query.numRowsAffected() == 1;
}

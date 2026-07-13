#include "CvRepository.hpp"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <stdexcept>

namespace {

CvDocument readDocument(const QSqlQuery& query)
{
    CvDocument document;
    document.id_ = query.value(QStringLiteral("id")).toString();
    document.originalFileName_ = query.value(QStringLiteral("original_file_name")).toString();
    document.storedFileName_ = query.value(QStringLiteral("stored_file_name")).toString();
    document.fileName_ = document.originalFileName_;
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
    document.lastModifiedLabel_ = QDateTime::fromString(document.updatedAt_, Qt::ISODate).toLocalTime().date().toString(QStringLiteral("MMM d, yyyy"));
    document.fileSizeLabel_ = QStringLiteral("%1 KB").arg((document.sizeBytes_ + 1023) / 1024);
    document.categoryAccent_ = QStringLiteral("#1687ff");
    document.languageAccent_ = QStringLiteral("#65bf4c");
    return document;
}

QString selectSql()
{
    return QStringLiteral(
        "SELECT id, original_file_name, stored_file_name, relative_path, sha256, size_bytes,"
        " title, category, language, description, is_favorite, created_at, updated_at FROM cvs");
}

void throwQueryError(const QSqlQuery& query)
{
    throw std::runtime_error(query.lastError().text().toStdString());
}

QString sqlText(const QString& value)
{
    return value.isNull() ? QStringLiteral("") : value;
}

}

CvRepository::CvRepository(QSqlDatabase& database)
    : database_(database)
{
}

QVector<CvDocument> CvRepository::findAll() const
{
    QSqlQuery query(database_);
    if (!query.exec(selectSql() + QStringLiteral(" ORDER BY created_at DESC"))) {
        throwQueryError(query);
    }

    QVector<CvDocument> documents;
    while (query.next()) {
        auto document = readDocument(query);
        QSqlQuery links(database_);
        links.prepare(QStringLiteral("SELECT id FROM jobs WHERE cv_id = ? ORDER BY created_at"));
        links.addBindValue(document.id_);
        if (!links.exec()) {
            throwQueryError(links);
        }
        while (links.next()) {
            document.linkedApplicationIds_.append(links.value(0).toString());
        }
        documents.append(std::move(document));
    }
    return documents;
}

std::optional<CvDocument> CvRepository::findBySha256(const QString& sha256) const
{
    QSqlQuery query(database_);
    query.prepare(selectSql() + QStringLiteral(" WHERE sha256 = ?"));
    query.addBindValue(sha256);
    if (!query.exec()) {
        throwQueryError(query);
    }
    if (!query.next()) {
        return std::nullopt;
    }
    return readDocument(query);
}

void CvRepository::insert(const CvDocument& document) const
{
    QSqlQuery query(database_);
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
    query.addBindValue(sqlText(document.title_));
    query.addBindValue(sqlText(document.category_));
    query.addBindValue(sqlText(document.language_));
    query.addBindValue(sqlText(document.description_));
    query.addBindValue(document.isFavorite_);
    query.addBindValue(document.createdAt_);
    query.addBindValue(document.updatedAt_);
    if (!query.exec()) {
        throwQueryError(query);
    }
}

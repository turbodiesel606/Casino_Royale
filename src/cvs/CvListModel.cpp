#include "CvListModel.hpp"

#include "common/ModelPresentation.hpp"
#include "common/ModelRoleUtils.hpp"

#include <utility>

namespace {

QString linkedApplicationCountLabel(int count)
{
    return common::presentation::countLabel(
        count,
        QStringLiteral("job"),
        QStringLiteral("jobs"));
}

QString fileSizeLabel(qint64 sizeBytes)
{
    return QStringLiteral("%1 KB").arg((sizeBytes + 1023) / 1024);
}

QVariant roleValue(const CvDocument& cv, int role)
{
    switch (role) {
    case CvListModel::IdRole:
        return cv.id_;
    case CvListModel::FileNameRole:
        return cv.originalFileName_;
    case CvListModel::TitleRole:
        return cv.title_;
    case CvListModel::CategoryRole:
        return cv.category_;
    case CvListModel::CategoryAccentRole:
        return QStringLiteral("#1687ff");
    case CvListModel::LanguageRole:
        return cv.language_;
    case CvListModel::LanguageAccentRole:
        return QStringLiteral("#65bf4c");
    case CvListModel::LastModifiedLabelRole:
        return common::presentation::shortLocalDateLabel(cv.updatedAt_);
    case CvListModel::FileSizeLabelRole:
        return fileSizeLabel(cv.sizeBytes_);
    case CvListModel::DescriptionRole:
        return cv.description_;
    case CvListModel::LinkedApplicationCountRole:
        return cv.linkedApplicationIds_.size();
    case CvListModel::LinkedApplicationCountLabelRole:
        return linkedApplicationCountLabel(cv.linkedApplicationIds_.size());
    case CvListModel::IsFavoriteRole:
        return cv.isFavorite_;
    case CvListModel::CreatedAtRole:
        return cv.createdAt_;
    case CvListModel::UpdatedAtRole:
        return cv.updatedAt_;
    case CvListModel::IsArchivedRole:
        return cv.archivedAt_.isValid();
    case CvListModel::ArchivedAtRole:
        return cv.archivedAt_;
    default:
        return {};
    }
}

}

CvListModel::CvListModel(QObject* parent)
    : CvListModel(QVector<CvDocument>{}, parent)
{
}

CvListModel::CvListModel(QVector<CvDocument> documents, QObject* parent)
    : cvs_(std::move(documents))
    , QAbstractListModel(parent)
{
}

int CvListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : cvs_.size();
}

QVariant CvListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= cvs_.size()) {
        return {};
    }
    return roleValue(cvs_.at(index.row()), role);
}

QHash<int, QByteArray> CvListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {FileNameRole, "fileName"},
        {TitleRole, "title"},
        {CategoryRole, "category"},
        {CategoryAccentRole, "categoryAccent"},
        {LanguageRole, "language"},
        {LanguageAccentRole, "languageAccent"},
        {LastModifiedLabelRole, "lastModifiedLabel"},
        {FileSizeLabelRole, "fileSizeLabel"},
        {DescriptionRole, "description"},
        {LinkedApplicationCountRole, "linkedApplicationCount"},
        {LinkedApplicationCountLabelRole, "linkedApplicationCountLabel"},
        {IsFavoriteRole, "isFavorite"},
        {CreatedAtRole, "createdAt"},
        {UpdatedAtRole, "updatedAt"},
        {IsArchivedRole, "isArchived"},
        {ArchivedAtRole, "archivedAt"},
    };
}

const CvDocument* CvListModel::cvAt(int row) const
{
    return row >= 0 && row < cvs_.size() ? &cvs_.at(row) : nullptr;
}

int CvListModel::rowForId(const QString& cvId) const
{
    return common::model::rowForStringRoleValue(*this, IdRole, cvId);
}

const CvDocument* CvListModel::cvById(const QString& cvId) const
{
    return cvAt(rowForId(cvId));
}

void CvListModel::appendDocument(CvDocument document)
{
    const auto row = cvs_.size();
    beginInsertRows({}, row, row);
    cvs_.append(std::move(document));
    endInsertRows();
}

bool CvListModel::addLinkedApplication(const QString& cvId, const QString& applicationId)
{
    const auto row = rowForId(cvId);
    if (row < 0 || cvs_[row].linkedApplicationIds_.contains(applicationId)) {
        return false;
    }
    cvs_[row].linkedApplicationIds_.append(applicationId);
    const auto modelIndex = index(row, 0);
    emit dataChanged(
        modelIndex,
        modelIndex,
        {LinkedApplicationCountRole, LinkedApplicationCountLabelRole});
    return true;
}

bool CvListModel::removeLinkedApplication(
    const QString& cvId,
    const QString& applicationId)
{
    const auto row = rowForId(cvId);
    if (row < 0 || cvs_[row].linkedApplicationIds_.removeAll(applicationId) <= 0) {
        return false;
    }

    const auto modelIndex = index(row, 0);
    emit dataChanged(
        modelIndex,
        modelIndex,
        {LinkedApplicationCountRole, LinkedApplicationCountLabelRole});
    return true;
}

void CvListModel::removeLinkedApplications(const QStringList& applicationIds)
{
    if (applicationIds.isEmpty()) {
        return;
    }
    for (int row = 0; row < cvs_.size(); ++row) {
        auto& document = cvs_[row];
        bool changed = false;
        for (const auto& applicationId : applicationIds) {
            changed = document.linkedApplicationIds_.removeAll(applicationId) > 0 || changed;
        }
        if (changed) {
            const auto modelIndex = index(row, 0);
            emit dataChanged(
                modelIndex,
                modelIndex,
                {LinkedApplicationCountRole, LinkedApplicationCountLabelRole});
        }
    }
}

bool CvListModel::setFavorite(
    const QString& cvId,
    bool isFavorite,
    const QDateTime& updatedAt)
{
    const auto row = rowForId(cvId);
    if (row < 0) {
        return false;
    }
    cvs_[row].isFavorite_ = isFavorite;
    cvs_[row].updatedAt_ = updatedAt;
    const auto modelIndex = index(row, 0);
    emit dataChanged(
        modelIndex,
        modelIndex,
        {IsFavoriteRole, LastModifiedLabelRole, UpdatedAtRole});
    return true;
}

bool CvListModel::setArchiveState(
    const QString& cvId,
    const QDateTime& archivedAt,
    const QDateTime& updatedAt)
{
    const auto row = rowForId(cvId);
    if (row < 0) {
        return false;
    }
    cvs_[row].applyArchiveState(archivedAt, updatedAt);
    const auto modelIndex = index(row, 0);
    emit dataChanged(
        modelIndex,
        modelIndex,
        {IsArchivedRole, ArchivedAtRole, LastModifiedLabelRole, UpdatedAtRole});
    return true;
}

int CvListModel::removeDocuments(const QStringList& cvIds)
{
    int removed = 0;
    for (int row = cvs_.size() - 1; row >= 0; --row) {
        if (!cvIds.contains(cvs_.at(row).id_)) {
            continue;
        }
        beginRemoveRows({}, row, row);
        cvs_.removeAt(row);
        endRemoveRows();
        ++removed;
    }
    return removed;
}

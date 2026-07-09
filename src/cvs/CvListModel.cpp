#include "CvListModel.h"

#include <utility>

namespace {

QString linkedApplicationCountLabel(int count)
{
    return count == 1 ? QStringLiteral("1 job") : QStringLiteral("%1 jobs").arg(count);
}

QVariant roleValue(const CvDocument& cv, int role)
{
    switch (role) {
    case CvListModel::IdRole:
        return cv.id_;
    case CvListModel::FileNameRole:
        return cv.fileName_;
    case CvListModel::TitleRole:
        return cv.title_;
    case CvListModel::CategoryRole:
        return cv.category_;
    case CvListModel::CategoryAccentRole:
        return cv.categoryAccent_;
    case CvListModel::LanguageRole:
        return cv.language_;
    case CvListModel::LanguageAccentRole:
        return cv.languageAccent_;
    case CvListModel::LastModifiedLabelRole:
        return cv.lastModifiedLabel_;
    case CvListModel::FileSizeLabelRole:
        return cv.fileSizeLabel_;
    case CvListModel::DescriptionRole:
        return cv.description_;
    case CvListModel::LinkedApplicationCountRole:
        return cv.linkedApplicationIds_.size();
    case CvListModel::LinkedApplicationCountLabelRole:
        return linkedApplicationCountLabel(cv.linkedApplicationIds_.size());
    case CvListModel::IsFavoriteRole:
        return cv.isFavorite_;
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
    : QAbstractListModel(parent)
    , cvs_(std::move(documents))
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
    };
}

const CvDocument* CvListModel::cvAt(int row) const
{
    return row >= 0 && row < cvs_.size() ? &cvs_.at(row) : nullptr;
}

void CvListModel::setDocuments(QVector<CvDocument> documents)
{
    beginResetModel();
    cvs_ = std::move(documents);
    endResetModel();
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
    for (int row = 0; row < cvs_.size(); ++row) {
        auto& document = cvs_[row];
        if (document.id_ == cvId && !document.linkedApplicationIds_.contains(applicationId)) {
            document.linkedApplicationIds_.append(applicationId);
            const auto modelIndex = index(row, 0);
            emit dataChanged(modelIndex, modelIndex, {LinkedApplicationCountRole, LinkedApplicationCountLabelRole});
            return true;
        }
    }
    return false;
}

bool CvListModel::toggleFavorite(const QString& cvId)
{
    for (int row = 0; row < cvs_.size(); ++row) {
        if (cvs_[row].id_ == cvId) {
            cvs_[row].isFavorite_ = !cvs_[row].isFavorite_;
            const auto modelIndex = index(row, 0);
            emit dataChanged(modelIndex, modelIndex, {IsFavoriteRole});
            return true;
        }
    }
    return false;
}

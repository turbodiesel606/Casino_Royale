#include "CvListModel.h"

#include <utility>

namespace {

QString linkedApplicationCountLabel(int count)
{
    return count == 1 ? QStringLiteral("1 job") : QStringLiteral("%1 jobs").arg(count);
}

CvDocument makeCv(
    QString id,
    QString fileName,
    QString title,
    QString category,
    QString categoryAccent,
    QString language,
    QString languageAccent,
    QString lastModifiedLabel,
    QString fileSizeLabel,
    QString description,
    QStringList linkedApplicationIds,
    bool isFavorite)
{
    CvDocument cv;
    cv.id_ = std::move(id);
    cv.fileName_ = std::move(fileName);
    cv.title_ = std::move(title);
    cv.category_ = std::move(category);
    cv.categoryAccent_ = std::move(categoryAccent);
    cv.language_ = std::move(language);
    cv.languageAccent_ = std::move(languageAccent);
    cv.lastModifiedLabel_ = std::move(lastModifiedLabel);
    cv.fileSizeLabel_ = std::move(fileSizeLabel);
    cv.description_ = std::move(description);
    cv.linkedApplicationIds_ = std::move(linkedApplicationIds);
    cv.isFavorite_ = isFavorite;
    return cv;
}

QVector<CvDocument> makeSeedCvs()
{
    return {
        makeCv(
            QStringLiteral("cv-qt-2026"),
            QStringLiteral("CV_Qt_2026.pdf"),
            QStringLiteral("Qt/QML Engineer"),
            QStringLiteral("Qt/QML Developer"),
            QStringLiteral("#1687ff"),
            QStringLiteral("English"),
            QStringLiteral("#65bf4c"),
            QStringLiteral("May 12, 2026"),
            QStringLiteral("612 KB"),
            QStringLiteral("CV focused on Qt/QML development, desktop applications, and cross-platform experience."),
            {QStringLiteral("job-kdab-cpp-qt"), QStringLiteral("job-techsoft-qt-qml"), QStringLiteral("job-codecraft-cpp-qt"), QStringLiteral("job-innotech-qt-qml")},
            true),
        makeCv(
            QStringLiteral("cv-embedded"),
            QStringLiteral("CV_Embedded.pdf"),
            QStringLiteral("Embedded C++ Engineer"),
            QStringLiteral("Embedded Developer"),
            QStringLiteral("#16c5dd"),
            QStringLiteral("English"),
            QStringLiteral("#65bf4c"),
            QStringLiteral("May 5, 2026"),
            QStringLiteral("584 KB"),
            QStringLiteral("CV focused on embedded C++, hardware-adjacent products, and performance-sensitive desktop tooling."),
            {QStringLiteral("job-vision-embedded"), QStringLiteral("job-devsolutions-embedded")},
            false),
        makeCv(
            QStringLiteral("cv-general"),
            QStringLiteral("CV_General.pdf"),
            QStringLiteral("General Software Engineer"),
            QStringLiteral("General"),
            QStringLiteral("#7f8b98"),
            QStringLiteral("English"),
            QStringLiteral("#65bf4c"),
            QStringLiteral("Apr 28, 2026"),
            QStringLiteral("548 KB"),
            QStringLiteral("General-purpose software engineering CV used for broader C++ and product engineering vacancies."),
            {QStringLiteral("job-greenwidget-software"), QStringLiteral("job-byteworks-software"), QStringLiteral("job-platforma-cpp")},
            false),
        makeCv(
            QStringLiteral("cv-backend"),
            QStringLiteral("CV_Backend.pdf"),
            QStringLiteral("Backend Developer"),
            QStringLiteral("Backend Developer"),
            QStringLiteral("#b36bff"),
            QStringLiteral("English"),
            QStringLiteral("#65bf4c"),
            QStringLiteral("Apr 18, 2026"),
            QStringLiteral("536 KB"),
            QStringLiteral("Backend-focused CV for service development, APIs, deployment, and database-heavy product work."),
            {QStringLiteral("job-nexora-backend")},
            false),
    };
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
    : QAbstractListModel(parent)
    , cvs_(makeSeedCvs())
{
}

int CvListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return cvs_.size();
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
    if (row < 0 || row >= cvs_.size()) {
        return nullptr;
    }
    return &cvs_.at(row);
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

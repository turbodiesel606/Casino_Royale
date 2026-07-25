#ifndef JOBTRACKER_SRC_CVS_CVLISTMODEL_HPP
#define JOBTRACKER_SRC_CVS_CVLISTMODEL_HPP

#include "CvDocument.hpp"

#include <QAbstractListModel>
#include <QVector>

class CvListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        FileNameRole,
        TitleRole,
        CategoryRole,
        CategoryAccentRole,
        LanguageRole,
        LanguageAccentRole,
        LastModifiedLabelRole,
        FileSizeLabelRole,
        DescriptionRole,
        LinkedApplicationCountRole,
        LinkedApplicationCountLabelRole,
        IsFavoriteRole
    };

    explicit CvListModel(QObject* parent = nullptr);
    explicit CvListModel(QVector<CvDocument> documents, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const CvDocument* cvAt(int row) const;
    void setDocuments(QVector<CvDocument> documents);
    void appendDocument(CvDocument document);
    bool addLinkedApplication(const QString& cvId, const QString& applicationId);
    bool setFavorite(const QString& cvId, bool isFavorite);

private:
    QVector<CvDocument> cvs_;
};

#endif // JOBTRACKER_SRC_CVS_CVLISTMODEL_HPP

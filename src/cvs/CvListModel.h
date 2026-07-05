#pragma once

#include "CvDocument.h"

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

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const CvDocument* cvAt(int row) const;
    bool toggleFavorite(const QString& cvId);

private:
    QVector<CvDocument> cvs_;
};

#pragma once

#include <QString>
#include <QStringList>

struct CvDocument
{
    QString id_;
    QString fileName_;
    QString originalFileName_;
    QString storedFileName_;
    QString relativePath_;
    QString sha256_;
    qint64 sizeBytes_ = 0;
    QString title_;
    QString category_;
    QString categoryAccent_;
    QString language_;
    QString languageAccent_;
    QString lastModifiedLabel_;
    QString fileSizeLabel_;
    QString description_;
    QStringList linkedApplicationIds_;
    bool isFavorite_ = false;
    QString createdAt_;
    QString updatedAt_;
};

#ifndef JOBTRACKER_SRC_CVS_CVDOCUMENT_HPP
#define JOBTRACKER_SRC_CVS_CVDOCUMENT_HPP

#include <QDateTime>
#include <QString>
#include <QStringList>

struct CvDocument
{
    void applyArchiveState(const QDateTime& archivedAt, const QDateTime& updatedAt)
    {
        archivedAt_ = archivedAt;
        updatedAt_ = updatedAt;
    }

    QString id_;
    QString originalFileName_;
    QString storedFileName_;
    QString relativePath_;
    QString sha256_;
    qint64 sizeBytes_ = 0;
    QString title_;
    QString category_;
    QString language_;
    QString description_;
    QStringList linkedApplicationIds_;
    bool isFavorite_ = false;
    QDateTime createdAt_;
    QDateTime updatedAt_;
    QDateTime archivedAt_;
};

#endif // JOBTRACKER_SRC_CVS_CVDOCUMENT_HPP

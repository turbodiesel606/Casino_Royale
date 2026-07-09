#pragma once

#include "CvDocument.h"

#include <QUrl>

class CvRepository;
class StoragePaths;

struct CvImportResult
{
    CvDocument document_;
    QString copiedFilePath_;
    bool wasInserted_ = false;
};

class CvImportService final
{
public:
    CvImportService(const StoragePaths& paths, CvRepository& repository);

    CvImportResult importDocument(const QUrl& sourceUrl) const;

private:
    const StoragePaths& paths_;
    CvRepository& repository_;
};

#ifndef JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP
#define JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP

#include "CvDocument.hpp"

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

#endif // JOBTRACKER_SRC_CVS_CVIMPORTSERVICE_HPP

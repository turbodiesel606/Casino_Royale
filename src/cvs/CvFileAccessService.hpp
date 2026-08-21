#ifndef JOBTRACKER_SRC_CVS_CVFILEACCESSSERVICE_HPP
#define JOBTRACKER_SRC_CVS_CVFILEACCESSSERVICE_HPP

#include "CvManagedPathResolver.hpp"

#include <QString>

struct CvDocument;
class StoragePaths;

struct CvFileAccessResult
{
    bool opened_ = false;
    QString message_;
};

// Validates and opens CV files that belong to JobTracker's managed data directory.
class CvFileAccessService final
{
public:
    explicit CvFileAccessService(const StoragePaths& paths);

    CvFileAccessResult openDocument(const CvDocument& document) const;

private:
    CvManagedPathResolver pathResolver_;
};

#endif // JOBTRACKER_SRC_CVS_CVFILEACCESSSERVICE_HPP

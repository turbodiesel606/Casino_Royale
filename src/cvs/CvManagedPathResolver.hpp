#ifndef JOBTRACKER_SRC_CVS_CVMANAGEDPATHRESOLVER_HPP
#define JOBTRACKER_SRC_CVS_CVMANAGEDPATHRESOLVER_HPP

#include <QString>

struct CvDocument;
class StoragePaths;

struct CvManagedPathResolution final
{
    QString absolutePath_;
    QString message_;
    bool valid_ = false;
    bool exists_ = false;
};

// Resolves persisted CV paths only when they name a top-level regular file in
// JobTracker's managed Resumes directory.
class CvManagedPathResolver final
{
public:
    explicit CvManagedPathResolver(const StoragePaths& paths);

    CvManagedPathResolution resolve(const CvDocument& document) const;

private:
    const StoragePaths& paths_;
};

#endif // JOBTRACKER_SRC_CVS_CVMANAGEDPATHRESOLVER_HPP

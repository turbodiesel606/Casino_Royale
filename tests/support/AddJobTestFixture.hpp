#ifndef JOBTRACKER_TESTS_SUPPORT_ADDJOBTESTFIXTURE_HPP
#define JOBTRACKER_TESTS_SUPPORT_ADDJOBTESTFIXTURE_HPP

#include "StorageTestFixtures.hpp"

#include "cvs/CvImportService.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvRepository.hpp"
#include "directory/CompanyRepository.hpp"
#include "jobs/AddJobService.hpp"
#include "jobs/AddJobWorker.hpp"
#include "jobs/JobApplicationDraft.hpp"
#include "jobs/JobRepository.hpp"

namespace testsupport {

inline JobApplicationDraft validJobDraft()
{
    JobApplicationDraft draft;
    draft.jobTitle_ = QStringLiteral("Qt Developer");
    draft.companyName_ = QStringLiteral("Example Company");
    draft.jobUrl_ = QStringLiteral("https://example.com/jobs/qt");
    draft.workFormat_ = QStringLiteral("Remote");
    draft.status_ = QStringLiteral("Applied");
    draft.appliedDate_ = QStringLiteral("2026-07-09");
    draft.techStack_ = {
        QStringLiteral("Qt"),
        QStringLiteral(" C++ "),
        QStringLiteral("qt"),
    };
    return draft;
}

inline QVariantMap validJobFormValues(const QString& jobTitle = QStringLiteral("Qt Developer"))
{
    return {
        {QStringLiteral("jobTitle"), jobTitle},
        {QStringLiteral("jobUrl"), QString()},
        {QStringLiteral("companyName"), QStringLiteral("Example Company")},
        {QStringLiteral("workFormat"), QStringLiteral("Remote")},
        {QStringLiteral("status"), QStringLiteral("Applied")},
        {QStringLiteral("appliedDate"), QStringLiteral("2026-08-04")},
    };
}

class AddJobTestFixture final
{
public:
    AddJobTestFixture()
        : database_{storage_.paths().databasePath()}
        , cvRepository_{database_.connection()}
        , companyRepository_{database_.connection()}
        , jobRepository_{database_.connection()}
        , fileStore_{storage_.paths()}
        , importer_{fileStore_, cvRepository_}
        , service_{
              database_.connection(),
              jobRepository_,
              companyRepository_,
              importer_}
    {
    }

    bool isValid() const
    {
        return storage_.isValid();
    }

    TemporaryStorageFixture storage_;
    SqliteDatabase database_;
    CvRepository cvRepository_;
    CompanyRepository companyRepository_;
    JobRepository jobRepository_;
    CvManagedFileStore fileStore_;
    CvImportService importer_;
    AddJobService service_;
};

class AddJobWorkerTestFixture final
{
public:
    AddJobWorkerTestFixture()
        : database_{storage_.paths().databasePath()}
        , cvRepository_{database_.connection()}
        , companyRepository_{database_.connection()}
        , jobRepository_{database_.connection()}
        , worker_{storage_.paths().dataDirectory()}
    {
    }

    bool isValid() const
    {
        return storage_.isValid();
    }

    TemporaryStorageFixture storage_;
    SqliteDatabase database_;
    CvRepository cvRepository_;
    CompanyRepository companyRepository_;
    JobRepository jobRepository_;
    AddJobWorker worker_;
};

} // namespace testsupport

#endif // JOBTRACKER_TESTS_SUPPORT_ADDJOBTESTFIXTURE_HPP

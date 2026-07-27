#ifndef JOBTRACKER_SRC_JOBS_ADDJOBSERVICE_HPP
#define JOBTRACKER_SRC_JOBS_ADDJOBSERVICE_HPP

#include "JobApplication.hpp"
#include "JobApplicationDraft.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvDocument.hpp"
#include "directory/Company.hpp"

#include <QVariantMap>
#include <QUrl>

#include <atomic>
#include <memory>

class CvImportService;
class CompanyRepository;
class JobRepository;
class QSqlDatabase;
class QThread;

struct AddJobResult
{
    bool success_ = false;
    QVariantMap fieldErrors_;
    QString message_;
    JobApplication application_;
    Company company_;
    CvDocument cvDocument_;
    bool cvWasInserted_ = false;
};

struct AddJobPreparationResult final
{
    bool success_ = false;
    bool cancelled_ = false;
    QVariantMap fieldErrors_;
    QString message_;
    NormalizedJobApplicationDraft draft_;
    std::shared_ptr<CvManagedFilePreparation> cvPreparation_;
};

class AddJobService final
{
public:
    AddJobService(
        QSqlDatabase& database,
        JobRepository& jobRepository,
        CompanyRepository& companyRepository,
        CvImportService& cvImportService);

    AddJobPreparationResult prepare(
        const JobApplicationDraft& draft,
        const QUrl& selectedCvUrl,
        const std::shared_ptr<std::atomic_bool>& cancellation) const;
    AddJobResult complete(AddJobPreparationResult preparation) const;
    AddJobResult create(const JobApplicationDraft& draft, const QUrl& selectedCvUrl) const;

private:
    QSqlDatabase& database_;
    JobRepository& jobRepository_;
    CompanyRepository& companyRepository_;
    CvImportService& cvImportService_;
    QThread* owningThread_ = nullptr;
};

#endif // JOBTRACKER_SRC_JOBS_ADDJOBSERVICE_HPP

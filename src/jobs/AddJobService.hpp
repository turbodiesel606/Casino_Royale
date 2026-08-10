#ifndef JOBTRACKER_SRC_JOBS_ADDJOBSERVICE_HPP
#define JOBTRACKER_SRC_JOBS_ADDJOBSERVICE_HPP

#include "JobApplication.hpp"
#include "JobApplicationDraft.hpp"
#include "common/CancellationState.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "cvs/CvDocument.hpp"
#include "directory/Company.hpp"

#include <QVariantMap>
#include <QUrl>

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

struct AddJobPreflightResult final
{
    NormalizedJobApplicationDraft draft_;
    QVariantMap fieldErrors_;
    QString message_;

    bool isValid() const;
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

// Coordinates the durable Add Job workflow across validation, CV import,
// company resolution, and job persistence. Canonical preflight is independent
// of storage; the production persistence instance is created and used wholly
// on the dedicated Add Job worker thread. One transaction prevents partial
// database changes, and completed CV files are cleaned up after ordinary
// failures.
class AddJobService final
{
public:
    AddJobService(
        QSqlDatabase& database,
        JobRepository& jobRepository,
        CompanyRepository& companyRepository,
        CvImportService& cvImportService);

    static AddJobPreflightResult preflight(
        const JobApplicationDraft& draft,
        const QUrl& selectedCvUrl);
    AddJobPreparationResult prepare(
        const NormalizedJobApplicationDraft& draft,
        const QUrl& selectedCvUrl,
        const std::shared_ptr<CancellationState>& cancellation) const;
    AddJobResult complete(
        AddJobPreparationResult preparation,
        const std::shared_ptr<CancellationState>& cancellation = {}) const;
    AddJobResult create(const JobApplicationDraft& draft, const QUrl& selectedCvUrl) const;

private:
    QSqlDatabase& database_;
    JobRepository& jobRepository_;
    CompanyRepository& companyRepository_;
    CvImportService& cvImportService_;
    QThread* owningThread_ = nullptr;
};

#endif // JOBTRACKER_SRC_JOBS_ADDJOBSERVICE_HPP

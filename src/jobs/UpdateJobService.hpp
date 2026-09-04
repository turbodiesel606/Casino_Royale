#ifndef JOBTRACKER_SRC_JOBS_UPDATEJOBSERVICE_HPP
#define JOBTRACKER_SRC_JOBS_UPDATEJOBSERVICE_HPP

#include "JobApplication.hpp"
#include "JobApplicationDraft.hpp"
#include "common/CancellationState.hpp"
#include "cvs/CvDocument.hpp"
#include "cvs/CvImportService.hpp"
#include "cvs/CvManagedFileStore.hpp"
#include "directory/Company.hpp"

#include <QUrl>
#include <QVariantMap>

#include <memory>
#include <optional>

class CompanyRepository;
class JobRepository;
class QSqlDatabase;
class QThread;

struct UpdateJobPreparationResult final
{
    bool success_ = false;
    bool cancelled_ = false;
    bool replacementCvRequested_ = false;
    QVariantMap fieldErrors_;
    QString message_;
    JobApplication existingApplication_;
    NormalizedJobApplicationDraft draft_;
    std::shared_ptr<CvManagedFilePreparation> cvPreparation_;
};

struct UpdateJobResult final
{
    bool success_ = false;
    QVariantMap fieldErrors_;
    QString message_;
    QString previousCvId_;
    JobApplication application_;
    Company company_;
    std::optional<CvDocument> replacementCvDocument_;
    CvImportDisposition cvImportDisposition_ = CvImportDisposition::ExistingActive;
};

// Coordinates a durable job edit while preserving the existing job identity
// and optionally importing a replacement CV through the canonical file path.
class UpdateJobService final
{
public:
    UpdateJobService(
        QSqlDatabase& database,
        JobRepository& jobRepository,
        CompanyRepository& companyRepository,
        CvImportService& cvImportService);

    UpdateJobPreparationResult prepare(
        const QString& applicationId,
        const NormalizedJobApplicationDraft& draft,
        const QUrl& replacementCvUrl,
        const std::shared_ptr<CancellationState>& cancellation) const;
    UpdateJobResult complete(
        UpdateJobPreparationResult preparation,
        const std::shared_ptr<CancellationState>& cancellation = {}) const;

private:
    QSqlDatabase& database_;
    JobRepository& jobRepository_;
    CompanyRepository& companyRepository_;
    CvImportService& cvImportService_;
    QThread* owningThread_ = nullptr;
};

#endif // JOBTRACKER_SRC_JOBS_UPDATEJOBSERVICE_HPP

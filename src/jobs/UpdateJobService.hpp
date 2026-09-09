#ifndef JOBTRACKER_SRC_JOBS_UPDATEJOBSERVICE_HPP
#define JOBTRACKER_SRC_JOBS_UPDATEJOBSERVICE_HPP
#include "JobApplicationFactory.hpp"
#include "JobApplicationValidator.hpp"
#include "JobRepository.hpp"
#include "common/ExceptionUtils.hpp"
#include "cvs/CvLockWrapper.hpp"
#include "directory/CompanyRepository.hpp"
#include "storage/SqlTransaction.hpp"

//
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
#include <QSqlDatabase>
#include <QThread>

#include <stdexcept>
#include <utility>

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
        CvImportService& cvImportService,
        CvLockWrapper& cvMutationQueue);

    UpdateJobPreparationResult prepare(
        const QString& applicationId,
        const NormalizedJobApplicationDraft& draft,
        const QUrl& replacementCvUrl,
        const std::shared_ptr<CancellationState>& cancellation) const;

    UpdateJobPreparationResult prepareValidated(
        const QString& applicationId,
        const NormalizedJobApplicationDraft& draft,
        const QUrl& replacementCvUrl,
        const std::shared_ptr<CancellationState>& cancellation) const;

    UpdateJobResult complete(
        UpdateJobPreparationResult preparation,
        const std::shared_ptr<CancellationState>& cancellation = {}) const;

private:
    enum class DraftValidationMode
    {
        RequiredValidation,
        AlreadyValidated
    };

    // this is last verdict, dont touch choose in compile time!
    template<DraftValidationMode ValidationMode>
    UpdateJobPreparationResult prepareImpl(
        const QString& applicationId,
        const NormalizedJobApplicationDraft& draft,
        const QUrl& replacementCvUrl,
        const std::shared_ptr<CancellationState>& cancellation) const
    {
        UpdateJobPreparationResult result;
        result.draft_ = draft;
        result.replacementCvRequested_ = !replacementCvUrl.isEmpty();

        if (QThread::currentThread() != owningThread_) {
            result.message_ =
                QStringLiteral("Job update preparation must run on its owning thread.");
            return result;
        }

        if (cancellation == nullptr) {
            result.message_ =
                QStringLiteral("The job update cancellation state is unavailable.");
            return result;
        }

        if (cancellation->isCancellationRequested()) {
            result.cancelled_ = true;
            result.message_ = QStringLiteral("The job update was canceled.");
            return result;
        }

        const auto existing = jobRepository_.findById(applicationId);
        if (!existing) {
            result.message_ =
                QStringLiteral("The job application no longer exists.");
            return result;
        }

        result.existingApplication_ = *existing;

        if constexpr (ValidationMode == DraftValidationMode::RequiredValidation) {
            const auto validation = JobApplicationValidator::validate(
                draft,
                !existing->cvId_.trimmed().isEmpty()
                || result.replacementCvRequested_);

            result.fieldErrors_ = validation.fieldErrors_;

            if (!validation.isValid()) {
                result.message_ =
                    QStringLiteral("Please correct the highlighted fields.");
                return result;
            }
        }

        if (!result.replacementCvRequested_) {
            result.success_ = true;
            return result;
        }

        const auto cvPreparation =
            cvImportService_.prepareDocument(replacementCvUrl, cancellation);

        result.cancelled_ = cvPreparation.cancelled_;
        result.message_ = cvPreparation.message_;
        result.cvPreparation_ = cvPreparation.preparation_;
        result.success_ = cvPreparation.succeeded();

        return result;
    }

    QSqlDatabase& database_;
    JobRepository& jobRepository_;
    CompanyRepository& companyRepository_;
    CvImportService& cvImportService_;
    CvLockWrapper& cvLock_;
    QThread* owningThread_ = nullptr;
};

#endif // JOBTRACKER_SRC_JOBS_UPDATEJOBSERVICE_HPP

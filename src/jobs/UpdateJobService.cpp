#include "UpdateJobService.hpp"

#include "JobApplicationFactory.hpp"
#include "JobApplicationValidator.hpp"
#include "JobRepository.hpp"
#include "directory/CompanyRepository.hpp"
#include "storage/SqlTransaction.hpp"

#include <QSqlDatabase>
#include <QThread>

#include <stdexcept>
#include <utility>

UpdateJobService::UpdateJobService(
    QSqlDatabase& database,
    JobRepository& jobRepository,
    CompanyRepository& companyRepository,
    CvImportService& cvImportService)
    : database_{database}
    , jobRepository_{jobRepository}
    , companyRepository_{companyRepository}
    , cvImportService_{cvImportService}
    , owningThread_{QThread::currentThread()}
{
}

bool UpdateJobPreflightResult::isValid() const
{
    return fieldErrors_.isEmpty();
}

UpdateJobPreflightResult UpdateJobService::preflight(
    const JobApplicationDraft& draft,
    bool hasCurrentCv,
    const QUrl& replacementCvUrl)
{
    UpdateJobPreflightResult result;
    result.draft_ = JobApplicationFactory::normalize(draft);
    const auto validation = JobApplicationValidator::validate(
        result.draft_,
        hasCurrentCv || !replacementCvUrl.isEmpty());
    result.fieldErrors_ = validation.fieldErrors_;
    if (!validation.isValid()) {
        result.message_ = QStringLiteral("Please correct the highlighted fields.");
    }
    return result;
}

UpdateJobPreparationResult UpdateJobService::prepare(
    const QString& applicationId,
    const NormalizedJobApplicationDraft& draft,
    const QUrl& replacementCvUrl,
    const std::shared_ptr<CancellationState>& cancellation) const
{
    UpdateJobPreparationResult result;
    result.draft_ = draft;
    result.replacementCvRequested_ = !replacementCvUrl.isEmpty();

    if (QThread::currentThread() != owningThread_) {
        result.message_ = QStringLiteral("Job update preparation must run on its owning thread.");
        return result;
    }
    if (cancellation == nullptr) {
        result.message_ = QStringLiteral("The job update cancellation state is unavailable.");
        return result;
    }
    if (cancellation->isCancellationRequested()) {
        result.cancelled_ = true;
        result.message_ = QStringLiteral("The job update was canceled.");
        return result;
    }

    const auto existing = jobRepository_.findById(applicationId);
    if (!existing) {
        result.message_ = QStringLiteral("The job application no longer exists.");
        return result;
    }
    result.existingApplication_ = *existing;

    const auto validation = JobApplicationValidator::validate(
        draft,
        !existing->cvId_.trimmed().isEmpty() || result.replacementCvRequested_);
    result.fieldErrors_ = validation.fieldErrors_;
    if (!validation.isValid()) {
        result.message_ = QStringLiteral("Please correct the highlighted fields.");
        return result;
    }

    if (!result.replacementCvRequested_) {
        result.success_ = true;
        return result;
    }

    const auto cvPreparation = cvImportService_.prepareDocument(replacementCvUrl, cancellation);
    result.cancelled_ = cvPreparation.cancelled_;
    result.message_ = cvPreparation.message_;
    result.cvPreparation_ = cvPreparation.preparation_;
    result.success_ = cvPreparation.succeeded();
    return result;
}

UpdateJobResult UpdateJobService::complete(
    UpdateJobPreparationResult preparation,
    const std::shared_ptr<CancellationState>& cancellation) const
{
    UpdateJobResult result;
    result.fieldErrors_ = preparation.fieldErrors_;
    if (!preparation.success_) {
        result.message_ = preparation.message_;
        return result;
    }
    if (QThread::currentThread() != owningThread_) {
        result.message_ = QStringLiteral("Job update persistence must run on its owning thread.");
        return result;
    }
    if (cancellation != nullptr && cancellation->isCancellationRequested()) {
        result.message_ = QStringLiteral("The job update was canceled.");
        return result;
    }

    QString completedFilePath;
    try {
        SqlTransaction transaction{database_, QStringLiteral("Update Job persistence")};
        const auto company = companyRepository_.findOrCreateByName(
            preparation.draft_.companyName_);

        std::optional<CvImportResult> cvImport;
        const CvDocument* replacementCv = nullptr;
        if (preparation.replacementCvRequested_) {
            cvImport = cvImportService_.importPreparedDocument(
                preparation.cvPreparation_,
                CvArchivedDuplicatePolicy::PreserveArchived);
            completedFilePath = cvImport->completedFilePath_;
            replacementCv = &cvImport->document_;
        }

        auto application = JobApplicationFactory::update(
            preparation.existingApplication_,
            preparation.draft_,
            company,
            replacementCv);
        if (!jobRepository_.update(application)) {
            throw std::runtime_error("The job application no longer exists.");
        }

        transaction.commit();

        result.success_ = true;
        result.message_ = QStringLiteral("Job application changes saved successfully.");
        result.previousCvId_ = preparation.existingApplication_.cvId_;
        result.application_ = std::move(application);
        result.company_ = company;
        if (cvImport) {
            result.replacementCvDocument_ = cvImport->document_;
            result.cvImportDisposition_ = cvImport->disposition_;
        }
    }
    catch (const std::exception& error) {
        result.message_ = QString::fromUtf8(error.what());
        if (!cvImportService_.removeCompletedFile(completedFilePath)) {
            result.message_.append(QStringLiteral(
                " The managed CV file could not be cleaned up and will be quarantined on restart."));
        }
    }
    return result;
}

#ifndef JOBTRACKER_SRC_JOBS_JOBAPPLICATIONVALIDATOR_HPP
#define JOBTRACKER_SRC_JOBS_JOBAPPLICATIONVALIDATOR_HPP

#include "JobApplication.hpp"
#include "JobApplicationDraft.hpp"

#include <QStringList>
#include <QVariantMap>

struct JobApplicationValidationResult
{
    QVariantMap fieldErrors_;

    bool isValid() const;
    QStringList messages() const;
};

struct JobApplicationPreflightResult final
{
    NormalizedJobApplicationDraft draft_;
    QVariantMap fieldErrors_;
    QString message_;

    bool isValid() const;
};

// Owns storage-independent draft preflight and the structured validation
// contract shared by job creation, updates, and stored applications.
class JobApplicationValidator final
{
public:
    static JobApplicationPreflightResult preflight(
        const JobApplicationDraft& draft,
        bool hasCv);
    static JobApplicationValidationResult validate(
        const NormalizedJobApplicationDraft& draft,
        const QUrl& selectedCvUrl);
    static JobApplicationValidationResult validate(
        const NormalizedJobApplicationDraft& draft,
        bool hasCv);
    static JobApplicationValidationResult validate(const JobApplication& application);
};

#endif // JOBTRACKER_SRC_JOBS_JOBAPPLICATIONVALIDATOR_HPP

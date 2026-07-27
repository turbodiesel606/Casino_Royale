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

// Applies the same structured validation contract to Add Job and stored jobs.
class JobApplicationValidator final
{
public:
    static JobApplicationValidationResult validate(
        const NormalizedJobApplicationDraft& draft,
        const QUrl& selectedCvUrl);
    static JobApplicationValidationResult validate(const JobApplication& application);
};

#endif // JOBTRACKER_SRC_JOBS_JOBAPPLICATIONVALIDATOR_HPP

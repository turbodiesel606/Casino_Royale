#ifndef JOBTRACKER_SRC_JOBS_JOBAPPLICATIONFACTORY_HPP
#define JOBTRACKER_SRC_JOBS_JOBAPPLICATIONFACTORY_HPP

#include "JobApplication.hpp"
#include "JobApplicationDraft.hpp"

struct Company;
struct CvDocument;

// Owns the canonical draft normalization and domain-object construction path.
class JobApplicationFactory final
{
public:
    static NormalizedJobApplicationDraft normalize(const JobApplicationDraft& draft);
    static JobApplication create(
        const NormalizedJobApplicationDraft& draft,
        const Company& company,
        const CvDocument& cvDocument);
};

#endif // JOBTRACKER_SRC_JOBS_JOBAPPLICATIONFACTORY_HPP

#ifndef JOBTRACKER_SRC_JOBS_ADDJOBSERVICE_HPP
#define JOBTRACKER_SRC_JOBS_ADDJOBSERVICE_HPP

#include "JobApplication.hpp"
#include "JobApplicationDraft.hpp"
#include "cvs/CvDocument.hpp"
#include "directory/Company.hpp"

#include <QVariantMap>
#include <QUrl>

class CvImportService;
class CompanyRepository;
class JobRepository;
class QSqlDatabase;

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

class AddJobService final
{
public:
    AddJobService(
        QSqlDatabase& database,
        JobRepository& jobRepository,
        CompanyRepository& companyRepository,
        CvImportService& cvImportService);

    AddJobResult create(const JobApplicationDraft& draft, const QUrl& selectedCvUrl) const;

private:
    QSqlDatabase& database_;
    JobRepository& jobRepository_;
    CompanyRepository& companyRepository_;
    CvImportService& cvImportService_;
};

#endif // JOBTRACKER_SRC_JOBS_ADDJOBSERVICE_HPP

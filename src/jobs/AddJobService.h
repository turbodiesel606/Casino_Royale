#pragma once

#include "JobApplication.h"
#include "JobApplicationDraft.h"
#include "cvs/CvDocument.h"

#include <QVariantMap>
#include <QUrl>

class CvImportService;
class JobRepository;
class QSqlDatabase;

struct AddJobResult
{
    bool success_ = false;
    QVariantMap fieldErrors_;
    QString message_;
    JobApplication application_;
    CvDocument cvDocument_;
    bool cvWasInserted_ = false;
};

class AddJobService final
{
public:
    AddJobService(QSqlDatabase& database, JobRepository& jobRepository, CvImportService& cvImportService);

    AddJobResult create(const JobApplicationDraft& draft, const QUrl& selectedCvUrl) const;

private:
    QSqlDatabase& database_;
    JobRepository& jobRepository_;
    CvImportService& cvImportService_;
};

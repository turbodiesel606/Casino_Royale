#ifndef JOBTRACKER_TESTS_SUPPORT_JOBAPPLICATIONTESTDATA_HPP
#define JOBTRACKER_TESTS_SUPPORT_JOBAPPLICATIONTESTDATA_HPP

#include "jobs/JobApplication.hpp"

#include <QVector>

#include <utility>

namespace testsupport {

inline JobApplication makeJobApplication(
    QString id,
    QString companyId,
    QString companyName,
    QString jobTitle,
    QString cvId,
    QString cvFileName,
    QDate appliedDate,
    JobStatus status,
    QString nextStep)
{
    JobApplication application;
    application.id_ = std::move(id);
    application.companyId_ = std::move(companyId);
    application.companyName_ = std::move(companyName);
    application.jobTitle_ = std::move(jobTitle);
    application.jobUrl_ = QUrl{QStringLiteral("https://example.test/jobs/%1").arg(application.id_)};
    application.workFormat_ = WorkFormat::Remote;
    application.city_ = QStringLiteral("Prague, Czech Republic");
    application.salary_ = QStringLiteral("$4,500");
    application.status_ = status;
    application.appliedDate_ = appliedDate;
    application.nextStep_ = std::move(nextStep);
    application.cvId_ = std::move(cvId);
    application.cvFileName_ = std::move(cvFileName);
    application.description_ = QStringLiteral("Test description for %1.").arg(application.jobTitle_);
    application.requirements_ = QStringLiteral("Test requirements");
    application.techStack_ = {QStringLiteral("C++"), QStringLiteral("Qt")};
    application.notes_ = QStringLiteral("Test notes");
    application.createdAt_ = QDateTime{appliedDate, QTime{10, 0}, Qt::UTC};
    application.updatedAt_ = application.createdAt_;
    return application;
}

inline QVector<JobApplication> makeJobApplications()
{
    return {
        makeJobApplication(QStringLiteral("job-kdab-cpp-qt"), QStringLiteral("company-kdab"), QStringLiteral("KDAB"), QStringLiteral("C++/Qt Developer"), QStringLiteral("cv-qt-2026"), QStringLiteral("CV_Qt_2026.pdf"), QDate{2026, 5, 12}, JobStatus::Applied, QStringLiteral("Test Task")),
        makeJobApplication(QStringLiteral("job-techsoft-qt-qml"), QStringLiteral("company-techsoft"), QStringLiteral("TechSoft"), QStringLiteral("Qt/QML Engineer"), QStringLiteral("cv-qt-2026"), QStringLiteral("CV_Qt_2026.pdf"), QDate{2026, 5, 8}, JobStatus::Interview, QStringLiteral("Interview")),
        makeJobApplication(QStringLiteral("job-vision-embedded"), QStringLiteral("company-vision-systems"), QStringLiteral("Vision Systems"), QStringLiteral("Embedded Developer"), QStringLiteral("cv-embedded"), QStringLiteral("CV_Embedded.pdf"), QDate{2026, 5, 5}, JobStatus::TestTask, QStringLiteral("Test Task")),
        makeJobApplication(QStringLiteral("job-greenwidget-software"), QStringLiteral("company-greenwidget"), QStringLiteral("GreenWidget"), QStringLiteral("Software Engineer"), QStringLiteral("cv-general"), QStringLiteral("CV_General.pdf"), QDate{2026, 4, 28}, JobStatus::Offer, QStringLiteral("Salary Discussion")),
        makeJobApplication(QStringLiteral("job-byteworks-software"), QStringLiteral("company-byteworks"), QStringLiteral("ByteWorks"), QStringLiteral("Software Engineer"), QStringLiteral("cv-general"), QStringLiteral("CV_General.pdf"), QDate{2026, 4, 10}, JobStatus::Rejected, QStringLiteral("-")),
        makeJobApplication(QStringLiteral("job-platforma-cpp"), QStringLiteral("company-platforma"), QStringLiteral("Platforma"), QStringLiteral("C++ Developer"), QStringLiteral("cv-general"), QStringLiteral("CV_General.pdf"), QDate{2026, 3, 28}, JobStatus::Interview, QStringLiteral("HR Interview")),
    };
}

} // namespace testsupport

#endif // JOBTRACKER_TESTS_SUPPORT_JOBAPPLICATIONTESTDATA_HPP

#pragma once

#include "jobs/JobApplication.h"

#include <QVector>

#include <utility>

namespace testsupport {

inline JobApplication makeJobApplication(
    QString id,
    QString companyId,
    QString companyName,
    QString companyInitials,
    QString companyAccent,
    QString jobTitle,
    QString cvId,
    QString cvFileName,
    QString dateLabel,
    QString status,
    QString nextStep)
{
    JobApplication application;
    application.id_ = std::move(id);
    application.companyId_ = std::move(companyId);
    application.companyName_ = std::move(companyName);
    application.companyInitials_ = std::move(companyInitials);
    application.companyAccent_ = std::move(companyAccent);
    application.jobTitle_ = std::move(jobTitle);
    application.jobUrl_ = QStringLiteral("https://example.test/jobs/%1").arg(application.id_);
    application.workFormat_ = QStringLiteral("Remote");
    application.city_ = QStringLiteral("Prague, Czech Republic");
    application.salary_ = QStringLiteral("$4,500");
    application.status_ = std::move(status);
    application.appliedDate_ = dateLabel;
    application.dateLabel_ = std::move(dateLabel);
    application.nextStep_ = std::move(nextStep);
    application.cvId_ = std::move(cvId);
    application.cvFileName_ = std::move(cvFileName);
    application.description_ = QStringLiteral("Test description for %1.").arg(application.jobTitle_);
    application.requirements_ = QStringLiteral("Test requirements");
    application.techStack_ = {QStringLiteral("C++"), QStringLiteral("Qt")};
    application.notes_ = QStringLiteral("Test notes");
    return application;
}

inline QVector<JobApplication> makeJobApplications()
{
    return {
        makeJobApplication(QStringLiteral("job-kdab-cpp-qt"), QStringLiteral("company-kdab"), QStringLiteral("KDAB"), QStringLiteral("KDAB"), QStringLiteral("#146ce0"), QStringLiteral("C++/Qt Developer"), QStringLiteral("cv-qt-2026"), QStringLiteral("CV_Qt_2026.pdf"), QStringLiteral("May 12, 2026"), QStringLiteral("Applied"), QStringLiteral("Test Task")),
        makeJobApplication(QStringLiteral("job-techsoft-qt-qml"), QStringLiteral("company-techsoft"), QStringLiteral("TechSoft"), QStringLiteral("Te"), QStringLiteral("#153046"), QStringLiteral("Qt/QML Engineer"), QStringLiteral("cv-qt-2026"), QStringLiteral("CV_Qt_2026.pdf"), QStringLiteral("May 8, 2026"), QStringLiteral("Interview"), QStringLiteral("Interview")),
        makeJobApplication(QStringLiteral("job-vision-embedded"), QStringLiteral("company-vision-systems"), QStringLiteral("Vision Systems"), QStringLiteral("Vi"), QStringLiteral("#153046"), QStringLiteral("Embedded Developer"), QStringLiteral("cv-embedded"), QStringLiteral("CV_Embedded.pdf"), QStringLiteral("May 5, 2026"), QStringLiteral("Test Task"), QStringLiteral("Test Task")),
        makeJobApplication(QStringLiteral("job-greenwidget-software"), QStringLiteral("company-greenwidget"), QStringLiteral("GreenWidget"), QStringLiteral("Gr"), QStringLiteral("#153046"), QStringLiteral("Software Engineer"), QStringLiteral("cv-general"), QStringLiteral("CV_General.pdf"), QStringLiteral("Apr 28, 2026"), QStringLiteral("Offer"), QStringLiteral("Salary Discussion")),
        makeJobApplication(QStringLiteral("job-byteworks-software"), QStringLiteral("company-byteworks"), QStringLiteral("ByteWorks"), QStringLiteral("By"), QStringLiteral("#153046"), QStringLiteral("Software Engineer"), QStringLiteral("cv-general"), QStringLiteral("CV_General.pdf"), QStringLiteral("Apr 10, 2026"), QStringLiteral("Rejected"), QStringLiteral("-")),
        makeJobApplication(QStringLiteral("job-platforma-cpp"), QStringLiteral("company-platforma"), QStringLiteral("Platforma"), QStringLiteral("Pl"), QStringLiteral("#153046"), QStringLiteral("C++ Developer"), QStringLiteral("cv-general"), QStringLiteral("CV_General.pdf"), QStringLiteral("Mar 28, 2026"), QStringLiteral("Interview"), QStringLiteral("HR Interview")),
    };
}

} // namespace testsupport

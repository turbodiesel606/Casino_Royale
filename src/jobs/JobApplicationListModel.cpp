#include "JobApplicationListModel.h"

namespace {

QString statusAccent(const QString& status)
{
    if (status == QStringLiteral("Interview")) {
        return QStringLiteral("#ffbd21");
    }
    if (status == QStringLiteral("Offer")) {
        return QStringLiteral("#38c86b");
    }
    if (status == QStringLiteral("Rejected")) {
        return QStringLiteral("#ff4b49");
    }
    if (status == QStringLiteral("Test Task")) {
        return QStringLiteral("#16c5dd");
    }
    return QStringLiteral("#c2c7cb");
}

JobApplication makeApplication(
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
    QString nextStep,
    QString workFormat,
    QString salary)
{
    JobApplication application;
    application.id_ = std::move(id);
    application.companyId_ = std::move(companyId);
    application.companyName_ = std::move(companyName);
    application.companyInitials_ = std::move(companyInitials);
    application.companyAccent_ = std::move(companyAccent);
    application.jobTitle_ = std::move(jobTitle);
    application.jobUrl_ = QStringLiteral("https://example.com/jobs/%1").arg(application.id_);
    application.workFormat_ = std::move(workFormat);
    application.city_ = QStringLiteral("Prague, Czech Republic");
    application.salary_ = std::move(salary);
    application.status_ = std::move(status);
    application.appliedDate_ = dateLabel;
    application.dateLabel_ = std::move(dateLabel);
    application.nextStep_ = std::move(nextStep);
    application.cvId_ = std::move(cvId);
    application.cvFileName_ = std::move(cvFileName);
    application.description_ = QStringLiteral(
        "We are looking for an experienced %1 to build cross-platform desktop applications used by millions of users.")
                                   .arg(application.jobTitle_);
    application.requirements_ = QStringLiteral(
        "5+ years of C++ development experience\nStrong knowledge of Qt, Widgets, QML\nExperience with CMake and modern C++");
    application.techStack_ = {QStringLiteral("C++"), QStringLiteral("Qt"), QStringLiteral("QML"), QStringLiteral("CMake")};
    application.notes_ = QStringLiteral("Applied via company website. Strong focus on Qt 6, QML, and cross-platform development.");
    return application;
}

QVector<JobApplication> makeSeedApplications()
{
    return {
        makeApplication(QStringLiteral("job-kdab-cpp-qt"), QStringLiteral("company-kdab"), QStringLiteral("KDAB"), QStringLiteral("KDAB"), QStringLiteral("#146ce0"), QStringLiteral("C++/Qt Developer"), QStringLiteral("cv-qt-2026"), QStringLiteral("CV_Qt_2026.pdf"), QStringLiteral("May 12, 2026"), QStringLiteral("Applied"), QStringLiteral("Test Task"), QStringLiteral("Remote"), QStringLiteral("$4,500")),
        makeApplication(QStringLiteral("job-techsoft-qt-qml"), QStringLiteral("company-techsoft"), QStringLiteral("TechSoft"), QStringLiteral("Te"), QStringLiteral("#153046"), QStringLiteral("Qt/QML Engineer"), QStringLiteral("cv-qt-2026"), QStringLiteral("CV_Qt_2026.pdf"), QStringLiteral("May 8, 2026"), QStringLiteral("Interview"), QStringLiteral("Interview"), QStringLiteral("Remote"), QStringLiteral("$4,300")),
        makeApplication(QStringLiteral("job-vision-embedded"), QStringLiteral("company-vision-systems"), QStringLiteral("Vision Systems"), QStringLiteral("Vi"), QStringLiteral("#153046"), QStringLiteral("Embedded Developer"), QStringLiteral("cv-embedded"), QStringLiteral("CV_Embedded.pdf"), QStringLiteral("May 5, 2026"), QStringLiteral("Test Task"), QStringLiteral("Test Task"), QStringLiteral("Hybrid"), QStringLiteral("$4,100")),
        makeApplication(QStringLiteral("job-greenwidget-software"), QStringLiteral("company-greenwidget"), QStringLiteral("GreenWidget"), QStringLiteral("Gr"), QStringLiteral("#153046"), QStringLiteral("Software Engineer"), QStringLiteral("cv-general"), QStringLiteral("CV_General.pdf"), QStringLiteral("Apr 28, 2026"), QStringLiteral("Offer"), QStringLiteral("Salary Discussion"), QStringLiteral("Remote"), QStringLiteral("$4,700")),
        makeApplication(QStringLiteral("job-codecraft-cpp-qt"), QStringLiteral("company-codecraft"), QStringLiteral("CodeCraft"), QStringLiteral("Co"), QStringLiteral("#153046"), QStringLiteral("C++/Qt Developer"), QStringLiteral("cv-qt-2026"), QStringLiteral("CV_Qt_2026.pdf"), QStringLiteral("Apr 22, 2026"), QStringLiteral("Interview"), QStringLiteral("Technical Interview"), QStringLiteral("Remote"), QStringLiteral("$4,500")),
        makeApplication(QStringLiteral("job-nexora-backend"), QStringLiteral("company-nexora"), QStringLiteral("Nexora"), QStringLiteral("Ne"), QStringLiteral("#153046"), QStringLiteral("Backend Developer"), QStringLiteral("cv-backend"), QStringLiteral("CV_Backend.pdf"), QStringLiteral("Apr 18, 2026"), QStringLiteral("Applied"), QStringLiteral("Screening Call"), QStringLiteral("Remote"), QStringLiteral("$4,000")),
        makeApplication(QStringLiteral("job-byteworks-software"), QStringLiteral("company-byteworks"), QStringLiteral("ByteWorks"), QStringLiteral("By"), QStringLiteral("#153046"), QStringLiteral("Software Engineer"), QStringLiteral("cv-general"), QStringLiteral("CV_General.pdf"), QStringLiteral("Apr 10, 2026"), QStringLiteral("Rejected"), QStringLiteral("-"), QStringLiteral("Hybrid"), QStringLiteral("$3,900")),
        makeApplication(QStringLiteral("job-innotech-qt-qml"), QStringLiteral("company-innotech"), QStringLiteral("Innotech"), QStringLiteral("In"), QStringLiteral("#153046"), QStringLiteral("Qt/QML Engiqneer"), QStringLiteral("cv-qt-2026"), QStringLiteral("CV_Qt_2026.pdf"), QStringLiteral("Apr 2, 2026"), QStringLiteral("Test Task"), QStringLiteral("Test Task"), QStringLiteral("Remote"), QStringLiteral("$4,200")),
        makeApplication(QStringLiteral("job-platforma-cpp"), QStringLiteral("company-platforma"), QStringLiteral("Platforma"), QStringLiteral("Pl"), QStringLiteral("#153046"), QStringLiteral("C++ Developer"), QStringLiteral("cv-general"), QStringLiteral("CV_General.pdf"), QStringLiteral("Mar 28, 2026"), QStringLiteral("Interview"), QStringLiteral("HR Interview"), QStringLiteral("Office"), QStringLiteral("$4,100")),
        makeApplication(QStringLiteral("job-devsolutions-embedded"), QStringLiteral("company-devsolutions"), QStringLiteral("DevSolutions"), QStringLiteral("De"), QStringLiteral("#153046"), QStringLiteral("Embedded C++ Engineer"), QStringLiteral("cv-embedded"), QStringLiteral("CV_Embedded.pdf"), QStringLiteral("Mar 20, 2026"), QStringLiteral("Applied"), QStringLiteral("Screening Call"), QStringLiteral("Hybrid"), QStringLiteral("$4,000")),
    };
}

QVariant roleValue(const JobApplication& application, int role)
{
    switch (role) {
    case JobApplicationListModel::IdRole:
        return application.id_;
    case JobApplicationListModel::CompanyIdRole:
        return application.companyId_;
    case JobApplicationListModel::CompanyNameRole:
        return application.companyName_;
    case JobApplicationListModel::CompanyInitialsRole:
        return application.companyInitials_;
    case JobApplicationListModel::CompanyAccentRole:
        return application.companyAccent_;
    case JobApplicationListModel::JobTitleRole:
        return application.jobTitle_;
    case JobApplicationListModel::JobUrlRole:
        return application.jobUrl_;
    case JobApplicationListModel::WorkFormatRole:
        return application.workFormat_;
    case JobApplicationListModel::CityRole:
        return application.city_;
    case JobApplicationListModel::SalaryRole:
        return application.salary_;
    case JobApplicationListModel::StatusRole:
    case JobApplicationListModel::StatusLabelRole:
        return application.status_;
    case JobApplicationListModel::StatusAccentRole:
        return statusAccent(application.status_);
    case JobApplicationListModel::AppliedDateRole:
        return application.appliedDate_;
    case JobApplicationListModel::DateLabelRole:
        return application.dateLabel_;
    case JobApplicationListModel::NextStepRole:
        return application.nextStep_;
    case JobApplicationListModel::CvIdRole:
        return application.cvId_;
    case JobApplicationListModel::CvFileNameRole:
        return application.cvFileName_;
    case JobApplicationListModel::DescriptionRole:
        return application.description_;
    case JobApplicationListModel::RequirementsRole:
        return application.requirements_;
    case JobApplicationListModel::TechStackRole:
        return application.techStack_;
    case JobApplicationListModel::NotesRole:
        return application.notes_;
    default:
        return {};
    }
}

}

JobApplicationListModel::JobApplicationListModel(QObject* parent)
    : QAbstractListModel(parent)
    , applications_(makeSeedApplications())
{
}

int JobApplicationListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return applications_.size();
}

QVariant JobApplicationListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= applications_.size()) {
        return {};
    }

    return roleValue(applications_.at(index.row()), role);
}

QHash<int, QByteArray> JobApplicationListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {CompanyIdRole, "companyId"},
        {CompanyNameRole, "companyName"},
        {CompanyInitialsRole, "companyInitials"},
        {CompanyAccentRole, "companyAccent"},
        {JobTitleRole, "jobTitle"},
        {JobUrlRole, "jobUrl"},
        {WorkFormatRole, "workFormat"},
        {CityRole, "city"},
        {SalaryRole, "salary"},
        {StatusRole, "status"},
        {StatusLabelRole, "statusLabel"},
        {StatusAccentRole, "statusAccent"},
        {AppliedDateRole, "appliedDate"},
        {DateLabelRole, "dateLabel"},
        {NextStepRole, "nextStep"},
        {CvIdRole, "cvId"},
        {CvFileNameRole, "cvFileName"},
        {DescriptionRole, "description"},
        {RequirementsRole, "requirements"},
        {TechStackRole, "techStack"},
        {NotesRole, "notes"},
    };
}

const JobApplication* JobApplicationListModel::applicationAt(int row) const
{
    if (row < 0 || row >= applications_.size()) {
        return nullptr;
    }
    return &applications_.at(row);
}

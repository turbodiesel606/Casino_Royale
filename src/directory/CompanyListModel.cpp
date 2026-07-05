#include "CompanyListModel.h"

#include <utility>

namespace {

QString countLabel(int count, const QString& singular, const QString& plural)
{
    return count == 1 ? QStringLiteral("1 %1").arg(singular) : QStringLiteral("%1 %2").arg(count).arg(plural);
}

Company makeCompany(
    QString id,
    QString name,
    QString website,
    QString logoText,
    QString logoAccent,
    int openJobCount,
    int contactCount,
    QString lastActivityLabel,
    QString description,
    QString notes)
{
    Company company;
    company.id_ = std::move(id);
    company.name_ = std::move(name);
    company.website_ = std::move(website);
    company.logoText_ = std::move(logoText);
    company.logoAccent_ = std::move(logoAccent);
    company.openJobCount_ = openJobCount;
    company.contactCount_ = contactCount;
    company.lastActivityLabel_ = std::move(lastActivityLabel);
    company.description_ = std::move(description);
    company.notes_ = std::move(notes);
    return company;
}

QVector<Company> makeSeedCompanies()
{
    return {
        makeCompany(
            QStringLiteral("company-kdab"),
            QStringLiteral("KDAB"),
            QStringLiteral("kdab.com"),
            QStringLiteral("KDAB"),
            QStringLiteral("#146ce0"),
            1,
            2,
            QStringLiteral("2 days ago"),
            QStringLiteral("KDAB is a software consultancy focused on Qt, C++, and cross-platform engineering."),
            QStringLiteral("Strong Qt and C++ expertise, active in the Qt community, good long-term target.")),
        makeCompany(
            QStringLiteral("company-techsoft"),
            QStringLiteral("TechSoft"),
            QStringLiteral("techsoft.com"),
            QStringLiteral("Te"),
            QStringLiteral("#153046"),
            1,
            1,
            QStringLiteral("5 days ago"),
            QStringLiteral("TechSoft builds desktop productivity software and internal tools with Qt/QML."),
            QStringLiteral("Interview pipeline is active. Follow up after the technical discussion.")),
        makeCompany(
            QStringLiteral("company-vision-systems"),
            QStringLiteral("Vision Systems"),
            QStringLiteral("visionsystems.com"),
            QStringLiteral("Vi"),
            QStringLiteral("#153046"),
            1,
            1,
            QStringLiteral("1 week ago"),
            QStringLiteral("Vision Systems works on embedded and image-processing products."),
            QStringLiteral("Embedded vacancy uses the dedicated embedded CV.")),
        makeCompany(
            QStringLiteral("company-codecraft"),
            QStringLiteral("CodeCraft"),
            QStringLiteral("codecraft.io"),
            QStringLiteral("Co"),
            QStringLiteral("#153046"),
            1,
            0,
            QStringLiteral("2 weeks ago"),
            QStringLiteral("CodeCraft is a software studio delivering C++ and Qt product work."),
            QStringLiteral("Potentially useful contact source still missing.")),
        makeCompany(
            QStringLiteral("company-greenwidget"),
            QStringLiteral("GreenWidget"),
            QStringLiteral("greenwidget.com"),
            QStringLiteral("Gr"),
            QStringLiteral("#153046"),
            1,
            0,
            QStringLiteral("3 weeks ago"),
            QStringLiteral("GreenWidget builds business applications for distributed teams."),
            QStringLiteral("Offer-stage application. Keep salary notes current.")),
    };
}

QVariant roleValue(const Company& company, int role)
{
    switch (role) {
    case CompanyListModel::IdRole:
        return company.id_;
    case CompanyListModel::NameRole:
        return company.name_;
    case CompanyListModel::WebsiteRole:
        return company.website_;
    case CompanyListModel::LogoTextRole:
        return company.logoText_;
    case CompanyListModel::LogoAccentRole:
        return company.logoAccent_;
    case CompanyListModel::OpenJobCountRole:
        return company.openJobCount_;
    case CompanyListModel::OpenJobCountLabelRole:
        return countLabel(company.openJobCount_, QStringLiteral("job"), QStringLiteral("jobs"));
    case CompanyListModel::ContactCountRole:
        return company.contactCount_;
    case CompanyListModel::ContactCountLabelRole:
        return countLabel(company.contactCount_, QStringLiteral("contact"), QStringLiteral("contacts"));
    case CompanyListModel::LastActivityLabelRole:
        return company.lastActivityLabel_;
    case CompanyListModel::DescriptionRole:
        return company.description_;
    case CompanyListModel::NotesRole:
        return company.notes_;
    default:
        return {};
    }
}

}

CompanyListModel::CompanyListModel(QObject* parent)
    : QAbstractListModel(parent)
    , companies_(makeSeedCompanies())
{
}

int CompanyListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return companies_.size();
}

QVariant CompanyListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= companies_.size()) {
        return {};
    }

    return roleValue(companies_.at(index.row()), role);
}

QHash<int, QByteArray> CompanyListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {WebsiteRole, "website"},
        {LogoTextRole, "logoText"},
        {LogoAccentRole, "logoAccent"},
        {OpenJobCountRole, "openJobCount"},
        {OpenJobCountLabelRole, "openJobCountLabel"},
        {ContactCountRole, "contactCount"},
        {ContactCountLabelRole, "contactCountLabel"},
        {LastActivityLabelRole, "lastActivityLabel"},
        {DescriptionRole, "description"},
        {NotesRole, "notes"},
    };
}

const Company* CompanyListModel::companyAt(int row) const
{
    if (row < 0 || row >= companies_.size()) {
        return nullptr;
    }
    return &companies_.at(row);
}

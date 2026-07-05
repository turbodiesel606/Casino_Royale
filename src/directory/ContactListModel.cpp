#include "ContactListModel.h"

#include <utility>

namespace {

ContactInteraction makeInteraction(QString type, QString title, QString timestampLabel, QString notes)
{
    ContactInteraction interaction;
    interaction.type_ = std::move(type);
    interaction.title_ = std::move(title);
    interaction.timestampLabel_ = std::move(timestampLabel);
    interaction.notes_ = std::move(notes);
    return interaction;
}

Contact makeContact(
    QString id,
    QString displayName,
    QString initials,
    QString avatarAccent,
    QString roleTitle,
    QString companyId,
    QString companyName,
    QString relatedApplicationId,
    QString relatedApplicationTitle,
    QString email,
    QString telegram,
    QString linkedin,
    QString lastContactLabel,
    QString notes,
    QVector<ContactInteraction> interactions)
{
    Contact contact;
    contact.id_ = std::move(id);
    contact.displayName_ = std::move(displayName);
    contact.initials_ = std::move(initials);
    contact.avatarAccent_ = std::move(avatarAccent);
    contact.roleTitle_ = std::move(roleTitle);
    contact.companyId_ = std::move(companyId);
    contact.companyName_ = std::move(companyName);
    contact.relatedApplicationId_ = std::move(relatedApplicationId);
    contact.relatedApplicationTitle_ = std::move(relatedApplicationTitle);
    contact.email_ = std::move(email);
    contact.telegram_ = std::move(telegram);
    contact.linkedin_ = std::move(linkedin);
    contact.lastContactLabel_ = std::move(lastContactLabel);
    contact.notes_ = std::move(notes);
    contact.interactions_ = std::move(interactions);
    return contact;
}

QVector<Contact> makeSeedContacts()
{
    return {
        makeContact(
            QStringLiteral("contact-anna-mueller"),
            QStringLiteral("Anna Mueller"),
            QStringLiteral("AM"),
            QStringLiteral("#dcc2ad"),
            QStringLiteral("HR Manager"),
            QStringLiteral("company-kdab"),
            QStringLiteral("KDAB"),
            QStringLiteral("job-kdab-cpp-qt"),
            QStringLiteral("C++/Qt Developer"),
            QStringLiteral("anna.mueller@kdab.com"),
            QStringLiteral("@anna_mueller_kdab"),
            QStringLiteral("linkedin.com/in/anna-mueller-kdab"),
            QStringLiteral("2 days ago"),
            QStringLiteral("Very positive screening call. Strong Qt and C++ background. Interested in long-term opportunities."),
            {
                makeInteraction(QStringLiteral("call"), QStringLiteral("Screening call"), QStringLiteral("May 14, 2026 at 10:30"), QStringLiteral("Discussed Qt role expectations and next steps.")),
                makeInteraction(QStringLiteral("email"), QStringLiteral("Follow up"), QStringLiteral("May 9, 2026 at 14:20"), QStringLiteral("Sent updated Qt/QML CV and availability.")),
                makeInteraction(QStringLiteral("linkedin"), QStringLiteral("LinkedIn connection"), QStringLiteral("May 7, 2026 at 09:15"), QStringLiteral("Connected after application submission.")),
            }),
        makeContact(
            QStringLiteral("contact-thomas-becker"),
            QStringLiteral("Thomas Becker"),
            QStringLiteral("TB"),
            QStringLiteral("#8a98a5"),
            QStringLiteral("Senior Software Engineer"),
            QStringLiteral("company-kdab"),
            QStringLiteral("KDAB"),
            QStringLiteral("job-kdab-cpp-qt"),
            QStringLiteral("C++/Qt Developer"),
            QStringLiteral("thomas.becker@kdab.com"),
            QString(),
            QStringLiteral("linkedin.com/in/thomas-becker-qt"),
            QStringLiteral("1 week ago"),
            QStringLiteral("Potential technical interviewer for Qt/C++ topics."),
            {
                makeInteraction(QStringLiteral("linkedin"), QStringLiteral("Profile review"), QStringLiteral("May 10, 2026 at 11:00"), QStringLiteral("Reviewed Qt background before technical discussion.")),
            }),
        makeContact(
            QStringLiteral("contact-maria-techsoft"),
            QStringLiteral("Maria Novak"),
            QStringLiteral("MN"),
            QStringLiteral("#8a98a5"),
            QStringLiteral("Talent Acquisition"),
            QStringLiteral("company-techsoft"),
            QStringLiteral("TechSoft"),
            QStringLiteral("job-techsoft-qt-qml"),
            QStringLiteral("Qt/QML Engineer"),
            QStringLiteral("maria.novak@techsoft.com"),
            QStringLiteral("@maria_techsoft"),
            QStringLiteral("linkedin.com/in/maria-novak-techsoft"),
            QStringLiteral("5 days ago"),
            QStringLiteral("Coordinating interview schedule and practical QML task."),
            {
                makeInteraction(QStringLiteral("email"), QStringLiteral("Interview schedule"), QStringLiteral("May 8, 2026 at 16:45"), QStringLiteral("Confirmed technical interview window.")),
            }),
        makeContact(
            QStringLiteral("contact-ivan-vision"),
            QStringLiteral("Ivan Petrov"),
            QStringLiteral("IP"),
            QStringLiteral("#8a98a5"),
            QStringLiteral("Engineering Manager"),
            QStringLiteral("company-vision-systems"),
            QStringLiteral("Vision Systems"),
            QStringLiteral("job-vision-embedded"),
            QStringLiteral("Embedded Developer"),
            QStringLiteral("ivan.petrov@visionsystems.com"),
            QStringLiteral("@ivan_embedded"),
            QString(),
            QStringLiteral("1 week ago"),
            QStringLiteral("Embedded role contact. Ask about device constraints and C++ standard support."),
            {
                makeInteraction(QStringLiteral("call"), QStringLiteral("Role clarification"), QStringLiteral("May 6, 2026 at 12:10"), QStringLiteral("Discussed embedded product scope.")),
            }),
    };
}

QVariant roleValue(const Contact& contact, int role)
{
    switch (role) {
    case ContactListModel::IdRole:
        return contact.id_;
    case ContactListModel::DisplayNameRole:
        return contact.displayName_;
    case ContactListModel::InitialsRole:
        return contact.initials_;
    case ContactListModel::AvatarAccentRole:
        return contact.avatarAccent_;
    case ContactListModel::RoleTitleRole:
        return contact.roleTitle_;
    case ContactListModel::CompanyIdRole:
        return contact.companyId_;
    case ContactListModel::CompanyNameRole:
        return contact.companyName_;
    case ContactListModel::RelatedApplicationIdRole:
        return contact.relatedApplicationId_;
    case ContactListModel::RelatedApplicationTitleRole:
        return contact.relatedApplicationTitle_;
    case ContactListModel::EmailRole:
        return contact.email_;
    case ContactListModel::TelegramRole:
        return contact.telegram_;
    case ContactListModel::LinkedinRole:
        return contact.linkedin_;
    case ContactListModel::LastContactLabelRole:
        return contact.lastContactLabel_;
    case ContactListModel::NotesRole:
        return contact.notes_;
    default:
        return {};
    }
}

}

ContactListModel::ContactListModel(QObject* parent)
    : QAbstractListModel(parent)
    , contacts_(makeSeedContacts())
{
}

int ContactListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return contacts_.size();
}

QVariant ContactListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= contacts_.size()) {
        return {};
    }

    return roleValue(contacts_.at(index.row()), role);
}

QHash<int, QByteArray> ContactListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {DisplayNameRole, "displayName"},
        {InitialsRole, "initials"},
        {AvatarAccentRole, "avatarAccent"},
        {RoleTitleRole, "roleTitle"},
        {CompanyIdRole, "companyId"},
        {CompanyNameRole, "companyName"},
        {RelatedApplicationIdRole, "relatedApplicationId"},
        {RelatedApplicationTitleRole, "relatedApplicationTitle"},
        {EmailRole, "email"},
        {TelegramRole, "telegram"},
        {LinkedinRole, "linkedin"},
        {LastContactLabelRole, "lastContactLabel"},
        {NotesRole, "notes"},
    };
}

const Contact* ContactListModel::contactAt(int row) const
{
    if (row < 0 || row >= contacts_.size()) {
        return nullptr;
    }
    return &contacts_.at(row);
}

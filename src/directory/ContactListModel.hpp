#ifndef JOBTRACKER_SRC_DIRECTORY_CONTACTLISTMODEL_HPP
#define JOBTRACKER_SRC_DIRECTORY_CONTACTLISTMODEL_HPP

#include "Contact.hpp"

#include <QAbstractListModel>
#include <QVector>

class ContactListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        DisplayNameRole,
        InitialsRole,
        AvatarAccentRole,
        RoleTitleRole,
        CompanyIdRole,
        CompanyNameRole,
        RelatedApplicationIdRole,
        RelatedApplicationTitleRole,
        EmailRole,
        TelegramRole,
        LinkedinRole,
        LastContactLabelRole,
        NotesRole
    };

    explicit ContactListModel(QObject* parent = nullptr);
    explicit ContactListModel(QVector<Contact> contacts, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const Contact* contactAt(int row) const;

private:
    QVector<Contact> contacts_;
};

#endif // JOBTRACKER_SRC_DIRECTORY_CONTACTLISTMODEL_HPP

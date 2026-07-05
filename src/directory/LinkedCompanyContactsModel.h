#pragma once

#include "ContactListModel.h"

#include <QAbstractListModel>
#include <QVector>

class LinkedCompanyContactsModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        DisplayNameRole,
        InitialsRole,
        AvatarAccentRole,
        RoleTitleRole,
        EmailRole,
        TelegramRole,
        LinkedinRole,
        LastContactLabelRole
    };

    explicit LinkedCompanyContactsModel(const ContactListModel& contactModel, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setCompanyId(const QString& companyId);

private:
    QVariant sourceData(int sourceRow, int role) const;
    void rebuildSourceRows();

    const ContactListModel& contactModel_;
    QString companyId_;
    QVector<int> sourceRows_;
};

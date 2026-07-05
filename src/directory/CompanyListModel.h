#pragma once

#include "Company.h"

#include <QAbstractListModel>
#include <QVector>

class CompanyListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        WebsiteRole,
        LogoTextRole,
        LogoAccentRole,
        OpenJobCountRole,
        OpenJobCountLabelRole,
        ContactCountRole,
        ContactCountLabelRole,
        LastActivityLabelRole,
        DescriptionRole,
        NotesRole
    };

    explicit CompanyListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const Company* companyAt(int row) const;

private:
    QVector<Company> companies_;
};

#ifndef JOBTRACKER_SRC_DIRECTORY_COMPANYLISTMODEL_HPP
#define JOBTRACKER_SRC_DIRECTORY_COMPANYLISTMODEL_HPP

#include "Company.hpp"

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
        NotesRole,
        CreatedAtRole,
        UpdatedAtRole
    };

    explicit CompanyListModel(QObject* parent = nullptr);
    explicit CompanyListModel(QVector<Company> companies, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const Company* companyAt(int row) const;
    int rowForId(const QString& companyId) const;
    bool upsertCompany(Company company);
    void setOpenJobCount(const QString& companyId, int count);

private:
    QVector<Company> companies_;
};

#endif // JOBTRACKER_SRC_DIRECTORY_COMPANYLISTMODEL_HPP

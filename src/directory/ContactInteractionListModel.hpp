#ifndef JOBTRACKER_SRC_DIRECTORY_CONTACTINTERACTIONLISTMODEL_HPP
#define JOBTRACKER_SRC_DIRECTORY_CONTACTINTERACTIONLISTMODEL_HPP

#include "Contact.hpp"

#include <QAbstractListModel>
#include <QVector>

class ContactInteractionListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        TypeRole = Qt::UserRole + 1,
        TitleRole,
        TimestampLabelRole,
        NotesRole
    };

    explicit ContactInteractionListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setInteractions(QVector<ContactInteraction> interactions);

private:
    QVector<ContactInteraction> interactions_;
};

#endif // JOBTRACKER_SRC_DIRECTORY_CONTACTINTERACTIONLISTMODEL_HPP

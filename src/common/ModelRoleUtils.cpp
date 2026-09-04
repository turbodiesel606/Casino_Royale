#include "ModelRoleUtils.hpp"

#include <QAbstractItemModel>
#include <QByteArray>
#include <QModelIndex>
#include <QString>

namespace common::model {

QVariant roleValueAt(
    const QAbstractItemModel& model,
    int row,
    int role)
{
    if (row < 0 || row >= model.rowCount()) {
        return {};
    }

    const auto modelIndex = model.index(row, 0);
    return modelIndex.isValid() ? model.data(modelIndex, role) : QVariant{};
}

QString stringRoleAt(
    const QAbstractItemModel& model,
    int row,
    int role)
{
    return roleValueAt(model, row, role).toString();
}

int rowForStringRoleValue(
    const QAbstractItemModel& model,
    int role,
    const QString& value)
{
    if (value.isEmpty()) {
        return -1;
    }

    for (int row = 0; row < model.rowCount(); ++row) {
        if (stringRoleAt(model, row, role) == value) {
            return row;
        }
    }

    return -1;
}

QVariantMap rowToVariantMap(
    const QAbstractItemModel& model,
    int row,
    std::initializer_list<int> roles)
{
    if (row < 0 || row >= model.rowCount()) {
        return {};
    }

    QVariantMap result;
    const auto roleNames = model.roleNames();
    for (const auto role : roles) {
        const auto roleName = roleNames.value(role);
        if (!roleName.isEmpty()) {
            result.insert(
                QString::fromUtf8(roleName),
                roleValueAt(model, row, role));
        }
    }
    return result;
}

} // namespace common::model

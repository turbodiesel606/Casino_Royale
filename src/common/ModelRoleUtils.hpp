#ifndef JOBTRACKER_SRC_COMMON_MODELROLEUTILS_HPP
#define JOBTRACKER_SRC_COMMON_MODELROLEUTILS_HPP

#include <QVariant>
#include <QVariantMap>

#include <initializer_list>

class QAbstractItemModel;
class QString;

namespace common::model {

QVariant roleValueAt(
    const QAbstractItemModel& model,
    int row,
    int role);

QString stringRoleAt(
    const QAbstractItemModel& model,
    int row,
    int role);

int rowForStringRoleValue(
    const QAbstractItemModel& model,
    int role,
    const QString& value);

/**
 * Builds a map from an explicit role whitelist. Invalid rows and unnamed
 * roles are omitted so internal model roles are never exposed accidentally.
 */
QVariantMap rowToVariantMap(
    const QAbstractItemModel& model,
    int row,
    std::initializer_list<int> roles);

} // namespace common::model

#endif // JOBTRACKER_SRC_COMMON_MODELROLEUTILS_HPP

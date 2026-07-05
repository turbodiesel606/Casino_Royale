#include "RoleFilterProxyModel.h"

#include <QDate>
#include <QLocale>
#include <QRegularExpression>
#include <QStringList>

#include <utility>


RoleFilterProxyModel::RoleFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

QString RoleFilterProxyModel::searchText() const
{
    return searchText_;
}

void RoleFilterProxyModel::setSearchText(const QString& searchText)
{
    const auto normalized = searchText.trimmed();
    if (searchText_ == normalized) {
        return;
    }

    searchText_ = normalized;
    invalidateFilter();
}

void RoleFilterProxyModel::setSearchRoles(QVector<int> roles)
{
    searchRoles_ = std::move(roles);
    invalidateFilter();
}

void RoleFilterProxyModel::setExactFilter(int role, const QString& value)
{
    const auto normalized = value.trimmed();
    if (exactFilters_.value(role) == normalized) {
        return;
    }

    if (normalized.isEmpty()) {
        exactFilters_.remove(role);
    } else {
        exactFilters_.insert(role, normalized);
    }
    invalidateFilter();
}

void RoleFilterProxyModel::clearExactFilter()
{
    if (exactFilters_.isEmpty()) {
        return;
    }

    exactFilters_.clear();
    invalidateFilter();
}

void RoleFilterProxyModel::setRequiredNonEmptyRole(int role)
{
    if (requiredNonEmptyRole_ == role) {
        return;
    }

    requiredNonEmptyRole_ = role;
    invalidateFilter();
}

void RoleFilterProxyModel::clearRequiredNonEmptyRole()
{
    if (requiredNonEmptyRole_ < 0) {
        return;
    }

    requiredNonEmptyRole_ = -1;
    invalidateFilter();
}

void RoleFilterProxyModel::setSort(int role, Qt::SortOrder order)
{
    setSortRole(role);
    sort(0, order);
}

bool RoleFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    return rowMatchesExactFilter(sourceRow, sourceParent) && rowMatchesRequiredNonEmptyRole(sourceRow, sourceParent)
        && rowMatchesSearch(sourceRow, sourceParent);
}

bool RoleFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    const auto leftData = sourceModel()->data(left, sortRole());
    const auto rightData = sourceModel()->data(right, sortRole());

    bool leftIsInt = false;
    bool rightIsInt = false;
    const auto leftInt = leftData.toInt(&leftIsInt);
    const auto rightInt = rightData.toInt(&rightIsInt);
    if (leftIsInt && rightIsInt) {
        return leftInt < rightInt;
    }

    const QLocale englishLocale(QLocale::English);
    const auto leftDate = englishLocale.toDate(leftData.toString(), QStringLiteral("MMM d, yyyy"));
    const auto rightDate = englishLocale.toDate(rightData.toString(), QStringLiteral("MMM d, yyyy"));
    if (leftDate.isValid() && rightDate.isValid()) {
        return leftDate < rightDate;
    }

    return QString::localeAwareCompare(leftData.toString().toCaseFolded(), rightData.toString().toCaseFolded()) < 0;
}

bool RoleFilterProxyModel::rowMatchesSearch(int sourceRow, const QModelIndex& sourceParent) const
{
    if (searchText_.isEmpty() || searchRoles_.isEmpty()) {
        return true;
    }

    const auto tokens = searchText_.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (tokens.isEmpty()) {
        return true;
    }

    QString haystack;
    const auto sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    for (const auto role : searchRoles_) {
        haystack += sourceModel()->data(sourceIndex, role).toString();
        haystack += QLatin1Char(' ');
    }
    const auto normalizedHaystack = haystack.toCaseFolded();

    for (const auto& token : tokens) {
        if (!normalizedHaystack.contains(token.toCaseFolded())) {
            return false;
        }
    }

    return true;
}

bool RoleFilterProxyModel::rowMatchesExactFilter(int sourceRow, const QModelIndex& sourceParent) const
{
    if (exactFilters_.isEmpty()) {
        return true;
    }

    const auto sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    for (auto it = exactFilters_.cbegin(); it != exactFilters_.cend(); ++it) {
        if (sourceModel()->data(sourceIndex, it.key()).toString().compare(it.value(), Qt::CaseInsensitive) != 0) {
            return false;
        }
    }

    return true;
}

bool RoleFilterProxyModel::rowMatchesRequiredNonEmptyRole(int sourceRow, const QModelIndex& sourceParent) const
{
    if (requiredNonEmptyRole_ < 0) {
        return true;
    }

    const auto sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    return !sourceModel()->data(sourceIndex, requiredNonEmptyRole_).toString().trimmed().isEmpty();
}

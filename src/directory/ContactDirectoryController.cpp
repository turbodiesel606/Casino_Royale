#include "ContactDirectoryController.hpp"

#include "common/ModelRoleUtils.hpp"

ContactDirectoryController::ContactDirectoryController(ContactListModel& contactModel, QObject* parent)
    : QObject(parent)
    , contactModel_(contactModel)
    , filteredContactModel_(this)
    , interactionHistoryModel_(this)
    , selectionTracker_(filteredContactModel_, ContactListModel::IdRole)
{
    filteredContactModel_.setSearchRoles({
        ContactListModel::DisplayNameRole,
        ContactListModel::RoleTitleRole,
        ContactListModel::CompanyNameRole,
        ContactListModel::RelatedApplicationTitleRole,
        ContactListModel::EmailRole,
        ContactListModel::TelegramRole,
        ContactListModel::LinkedinRole,
        ContactListModel::NotesRole,
    });
    connect(
        &selectionTracker_,
        &StableIdSelectionTracker::selectionChanged,
        this,
        &ContactDirectoryController::handleSelectionChanged);
    connect(
        &selectionTracker_,
        &StableIdSelectionTracker::visibleRowCountChanged,
        this,
        [this]() {
            emit contactCountChanged();
            emit resultSummaryChanged();
        });
    filteredContactModel_.setSourceModel(&contactModel_);
    selectionTracker_.synchronize();
}

QAbstractItemModel* ContactDirectoryController::contactModel()
{
    return &filteredContactModel_;
}

QAbstractItemModel* ContactDirectoryController::interactionHistoryModel()
{
    return &interactionHistoryModel_;
}

int ContactDirectoryController::contactCount() const
{
    return selectionTracker_.visibleRowCount();
}

int ContactDirectoryController::selectedContactIndex() const
{
    return selectionTracker_.selectedRow();
}

QString ContactDirectoryController::selectedContactId() const
{
    return selectionTracker_.selectedId();
}

QVariantMap ContactDirectoryController::selectedContact() const
{
    const auto sourceIndex = selectionTracker_.selectedSourceIndex();
    return sourceIndex.isValid()
        ? common::model::rowToVariantMap(
            contactModel_,
            sourceIndex.row(),
            {
                ContactListModel::IdRole,
                ContactListModel::DisplayNameRole,
                ContactListModel::InitialsRole,
                ContactListModel::AvatarAccentRole,
                ContactListModel::RoleTitleRole,
                ContactListModel::CompanyIdRole,
                ContactListModel::CompanyNameRole,
                ContactListModel::RelatedApplicationIdRole,
                ContactListModel::RelatedApplicationTitleRole,
                ContactListModel::EmailRole,
                ContactListModel::TelegramRole,
                ContactListModel::LinkedinRole,
                ContactListModel::LastContactLabelRole,
                ContactListModel::NotesRole,
            })
        : QVariantMap{};
}

QString ContactDirectoryController::searchText() const
{
    return filteredContactModel_.searchText();
}

QString ContactDirectoryController::companyFilter() const
{
    return companyFilter_;
}

QString ContactDirectoryController::channelFilter() const
{
    return channelFilter_;
}

QString ContactDirectoryController::sortMode() const
{
    return sortMode_;
}

QString ContactDirectoryController::resultSummary() const
{
    const auto count = contactCount();
    return count == 1 ? QStringLiteral("1 contact") : QStringLiteral("%1 contacts").arg(count);
}

void ContactDirectoryController::selectContact(int index)
{
    selectionTracker_.selectRow(index);
}

void ContactDirectoryController::setSearchText(const QString& text)
{
    const auto normalized = text.trimmed();
    if (searchText() == normalized) {
        return;
    }

    selectionTracker_.beginModelUpdate();
    filteredContactModel_.setSearchText(normalized);
    selectionTracker_.endModelUpdate();
    emit searchTextChanged();
}

void ContactDirectoryController::setCompanyFilter(const QString& company)
{
    const auto normalized = company.trimmed();
    if (companyFilter_ == normalized) {
        return;
    }

    companyFilter_ = normalized;
    selectionTracker_.beginModelUpdate();
    filteredContactModel_.setExactFilter(ContactListModel::CompanyNameRole, companyFilter_ == QStringLiteral("All") ? QString() : companyFilter_);
    selectionTracker_.endModelUpdate();
    emit companyFilterChanged();
}

void ContactDirectoryController::setChannelFilter(const QString& channel)
{
    const auto normalized = channel.trimmed();
    if (channelFilter_ == normalized) {
        return;
    }

    channelFilter_ = normalized;
    selectionTracker_.beginModelUpdate();
    if (channelFilter_ == QStringLiteral("Email")) {
        filteredContactModel_.setRequiredNonEmptyRole(ContactListModel::EmailRole);
    } else if (channelFilter_ == QStringLiteral("Telegram")) {
        filteredContactModel_.setRequiredNonEmptyRole(ContactListModel::TelegramRole);
    } else if (channelFilter_ == QStringLiteral("LinkedIn")) {
        filteredContactModel_.setRequiredNonEmptyRole(ContactListModel::LinkedinRole);
    } else {
        filteredContactModel_.clearRequiredNonEmptyRole();
    }
    selectionTracker_.endModelUpdate();
    emit channelFilterChanged();
}

void ContactDirectoryController::setSortMode(const QString& sortMode)
{
    const auto normalized = sortMode.trimmed().isEmpty() ? QStringLiteral("Name") : sortMode.trimmed();
    if (sortMode_ == normalized) {
        return;
    }

    sortMode_ = normalized;
    selectionTracker_.beginModelUpdate();
    if (sortMode_ == QStringLiteral("Company")) {
        filteredContactModel_.setSort(ContactListModel::CompanyNameRole, Qt::AscendingOrder);
    } else if (sortMode_ == QStringLiteral("Last Contact")) {
        filteredContactModel_.setSort(ContactListModel::LastContactLabelRole, Qt::AscendingOrder);
    } else {
        filteredContactModel_.setSort(ContactListModel::DisplayNameRole, Qt::AscendingOrder);
    }
    selectionTracker_.endModelUpdate();
    emit sortModeChanged();
}

void ContactDirectoryController::clearFilters()
{
    if (searchText().isEmpty() && companyFilter_.isEmpty() && channelFilter_.isEmpty()) {
        return;
    }

    const bool didSearchTextChange = !searchText().isEmpty();
    const bool didCompanyFilterChange = !companyFilter_.isEmpty();
    const bool didChannelFilterChange = !channelFilter_.isEmpty();
    companyFilter_.clear();
    channelFilter_.clear();
    selectionTracker_.beginModelUpdate();
    filteredContactModel_.setSearchText(QString());
    filteredContactModel_.clearExactFilter();
    filteredContactModel_.clearRequiredNonEmptyRole();
    selectionTracker_.endModelUpdate();
    if (didSearchTextChange) {
        emit searchTextChanged();
    }
    if (didCompanyFilterChange) {
        emit companyFilterChanged();
    }
    if (didChannelFilterChange) {
        emit channelFilterChanged();
    }
}

const Contact* ContactDirectoryController::selectedSourceContact() const
{
    const auto sourceIndex = selectionTracker_.selectedSourceIndex();
    return sourceIndex.isValid() ? contactModel_.contactAt(sourceIndex.row()) : nullptr;
}

void ContactDirectoryController::handleSelectionChanged(
    bool idChanged,
    bool rowChanged,
    bool dataChanged)
{
    if (idChanged) {
        updateInteractionHistory();
        emit selectedContactIdChanged();
    }
    if (rowChanged) {
        emit selectedContactIndexChanged();
    }
    if (idChanged || dataChanged) {
        emit selectedContactChanged();
    }
}

void ContactDirectoryController::updateInteractionHistory()
{
    const auto* contact = selectedSourceContact();
    interactionHistoryModel_.setInteractions(contact != nullptr ? contact->interactions_ : QVector<ContactInteraction>());
}

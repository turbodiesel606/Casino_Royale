#include "ContactDirectoryController.hpp"

ContactDirectoryController::ContactDirectoryController(ContactListModel& contactModel, QObject* parent)
    : QObject(parent)
    , contactModel_(contactModel)
    , filteredContactModel_(this)
    , interactionHistoryModel_(this)
{
    filteredContactModel_.setSourceModel(&contactModel_);
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
        &contactModel_,
        &QAbstractItemModel::rowsInserted,
        this,
        [this]() { refreshSelection(); });
    connect(
        &contactModel_,
        &QAbstractItemModel::rowsRemoved,
        this,
        [this]() { refreshSelection(); });
    connect(
        &contactModel_,
        &QAbstractItemModel::rowsMoved,
        this,
        [this]() { refreshSelection(); });
    connect(
        &contactModel_,
        &QAbstractItemModel::modelReset,
        this,
        [this]() { refreshSelection(!selectedContactId_.isEmpty()); });
    connect(
        &contactModel_,
        &QAbstractItemModel::layoutChanged,
        this,
        [this]() { refreshSelection(); });
    connect(
        &contactModel_,
        &QAbstractItemModel::dataChanged,
        this,
        [this](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
            const auto selectedRow = selectedSourceRow();
            refreshSelection(selectedRow >= topLeft.row() && selectedRow <= bottomRight.row());
        });
    refreshSelection();
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
    return filteredContactModel_.rowCount();
}

int ContactDirectoryController::selectedContactIndex() const
{
    return selectedContactIndex_;
}

QString ContactDirectoryController::selectedContactId() const
{
    return selectedContactId_;
}

QVariantMap ContactDirectoryController::selectedContact() const
{
    const auto* contact = selectedSourceContact();
    return contact != nullptr ? contactToMap(*contact) : QVariantMap();
}

QString ContactDirectoryController::searchText() const
{
    return searchText_;
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
    if (index < 0 || index >= filteredContactModel_.rowCount()) {
        return;
    }

    const auto contactId = filteredContactModel_.data(
        filteredContactModel_.index(index, 0),
        ContactListModel::IdRole).toString();
    const bool idChanged = contactId != selectedContactId_;
    if (!idChanged && index == selectedContactIndex_) {
        return;
    }

    selectedContactId_ = contactId;
    selectedContactIndex_ = index;
    if (idChanged) {
        updateInteractionHistory();
    }
    emit selectedContactChanged();
    if (idChanged) {
        emit interactionHistoryModelChanged();
    }
}

void ContactDirectoryController::setSearchText(const QString& text)
{
    const auto normalized = text.trimmed();
    if (searchText_ == normalized) {
        return;
    }

    searchText_ = normalized;
    filteredContactModel_.setSearchText(searchText_);
    refreshSelection();
    emit filtersChanged();
    emit contactModelChanged();
    emit resultSummaryChanged();
}

void ContactDirectoryController::setCompanyFilter(const QString& company)
{
    const auto normalized = company.trimmed();
    if (companyFilter_ == normalized) {
        return;
    }

    companyFilter_ = normalized;
    filteredContactModel_.setExactFilter(ContactListModel::CompanyNameRole, companyFilter_ == QStringLiteral("All") ? QString() : companyFilter_);
    refreshSelection();
    emit filtersChanged();
    emit contactModelChanged();
    emit resultSummaryChanged();
}

void ContactDirectoryController::setChannelFilter(const QString& channel)
{
    const auto normalized = channel.trimmed();
    if (channelFilter_ == normalized) {
        return;
    }

    channelFilter_ = normalized;
    if (channelFilter_ == QStringLiteral("Email")) {
        filteredContactModel_.setRequiredNonEmptyRole(ContactListModel::EmailRole);
    } else if (channelFilter_ == QStringLiteral("Telegram")) {
        filteredContactModel_.setRequiredNonEmptyRole(ContactListModel::TelegramRole);
    } else if (channelFilter_ == QStringLiteral("LinkedIn")) {
        filteredContactModel_.setRequiredNonEmptyRole(ContactListModel::LinkedinRole);
    } else {
        filteredContactModel_.clearRequiredNonEmptyRole();
    }
    refreshSelection();
    emit filtersChanged();
    emit contactModelChanged();
    emit resultSummaryChanged();
}

void ContactDirectoryController::setSortMode(const QString& sortMode)
{
    const auto normalized = sortMode.trimmed().isEmpty() ? QStringLiteral("Name") : sortMode.trimmed();
    if (sortMode_ == normalized) {
        return;
    }

    sortMode_ = normalized;
    if (sortMode_ == QStringLiteral("Company")) {
        filteredContactModel_.setSort(ContactListModel::CompanyNameRole, Qt::AscendingOrder);
    } else if (sortMode_ == QStringLiteral("Last Contact")) {
        filteredContactModel_.setSort(ContactListModel::LastContactLabelRole, Qt::AscendingOrder);
    } else {
        filteredContactModel_.setSort(ContactListModel::DisplayNameRole, Qt::AscendingOrder);
    }
    refreshSelection();
    emit filtersChanged();
    emit contactModelChanged();
}

void ContactDirectoryController::clearFilters()
{
    if (searchText_.isEmpty() && companyFilter_.isEmpty() && channelFilter_.isEmpty()) {
        return;
    }

    searchText_.clear();
    companyFilter_.clear();
    channelFilter_.clear();
    filteredContactModel_.setSearchText(QString());
    filteredContactModel_.clearExactFilter();
    filteredContactModel_.clearRequiredNonEmptyRole();
    refreshSelection();
    emit filtersChanged();
    emit contactModelChanged();
    emit resultSummaryChanged();
}

const Contact* ContactDirectoryController::selectedSourceContact() const
{
    const auto sourceRow = selectedSourceRow();
    return sourceRow >= 0 ? contactModel_.contactAt(sourceRow) : nullptr;
}

int ContactDirectoryController::selectedSourceRow() const
{
    if (selectedContactId_.isEmpty()) {
        return -1;
    }

    for (int row = 0; row < contactModel_.rowCount(); ++row) {
        const auto* contact = contactModel_.contactAt(row);
        if (contact != nullptr && contact->id_ == selectedContactId_) {
            return row;
        }
    }

    return -1;
}

void ContactDirectoryController::refreshSelection(bool selectedDataChanged)
{
    const auto previousId = selectedContactId_;
    const auto previousIndex = selectedContactIndex_;
    auto nextIndex = -1;

    if (!selectedContactId_.isEmpty()) {
        for (int row = 0; row < filteredContactModel_.rowCount(); ++row) {
            if (filteredContactModel_.data(
                    filteredContactModel_.index(row, 0),
                    ContactListModel::IdRole).toString() == selectedContactId_) {
                nextIndex = row;
                break;
            }
        }
    }

    if (nextIndex < 0) {
        if (filteredContactModel_.rowCount() > 0) {
            nextIndex = 0;
            selectedContactId_ = filteredContactModel_.data(
                filteredContactModel_.index(0, 0),
                ContactListModel::IdRole).toString();
        } else {
            selectedContactId_.clear();
        }
    }

    selectedContactIndex_ = nextIndex;
    const bool idChanged = selectedContactId_ != previousId;
    if (idChanged) {
        updateInteractionHistory();
    }
    if (idChanged || selectedContactIndex_ != previousIndex || selectedDataChanged) {
        emit selectedContactChanged();
    }
    if (idChanged) {
        emit interactionHistoryModelChanged();
    }
}

QVariantMap ContactDirectoryController::contactToMap(const Contact& contact) const
{
    return {
        {QStringLiteral("id"), contact.id_},
        {QStringLiteral("displayName"), contact.displayName_},
        {QStringLiteral("initials"), contact.initials_},
        {QStringLiteral("avatarAccent"), contact.avatarAccent_},
        {QStringLiteral("roleTitle"), contact.roleTitle_},
        {QStringLiteral("companyId"), contact.companyId_},
        {QStringLiteral("companyName"), contact.companyName_},
        {QStringLiteral("relatedApplicationId"), contact.relatedApplicationId_},
        {QStringLiteral("relatedApplicationTitle"), contact.relatedApplicationTitle_},
        {QStringLiteral("email"), contact.email_},
        {QStringLiteral("telegram"), contact.telegram_},
        {QStringLiteral("linkedin"), contact.linkedin_},
        {QStringLiteral("lastContactLabel"), contact.lastContactLabel_},
        {QStringLiteral("notes"), contact.notes_},
    };
}

void ContactDirectoryController::updateInteractionHistory()
{
    const auto* contact = selectedSourceContact();
    interactionHistoryModel_.setInteractions(contact != nullptr ? contact->interactions_ : QVector<ContactInteraction>());
}

#include "ContactDirectoryController.hpp"

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
    filteredContactModel_.setSourceModel(&contactModel_);
    selectionTracker_.synchronize();
    publishedContactCount_ = contactCount();
    connect(
        &filteredContactModel_,
        &QAbstractItemModel::rowsInserted,
        this,
        [this]() { handleVisibleCountChanged(); });
    connect(
        &filteredContactModel_,
        &QAbstractItemModel::rowsRemoved,
        this,
        [this]() { handleVisibleCountChanged(); });
    connect(
        &filteredContactModel_,
        &QAbstractItemModel::modelReset,
        this,
        [this]() { handleVisibleCountChanged(); });
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
    return selectionTracker_.selectedRow();
}

QString ContactDirectoryController::selectedContactId() const
{
    return selectionTracker_.selectedId();
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
    selectionTracker_.selectRow(index);
}

void ContactDirectoryController::setSearchText(const QString& text)
{
    const auto normalized = text.trimmed();
    if (searchText_ == normalized) {
        return;
    }

    searchText_ = normalized;
    selectionTracker_.beginModelUpdate();
    visibleCountNotificationsSuppressed_ = true;
    filteredContactModel_.setSearchText(searchText_);
    visibleCountNotificationsSuppressed_ = false;
    selectionTracker_.endModelUpdate();
    handleVisibleCountChanged();
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
    visibleCountNotificationsSuppressed_ = true;
    filteredContactModel_.setExactFilter(ContactListModel::CompanyNameRole, companyFilter_ == QStringLiteral("All") ? QString() : companyFilter_);
    visibleCountNotificationsSuppressed_ = false;
    selectionTracker_.endModelUpdate();
    handleVisibleCountChanged();
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
    visibleCountNotificationsSuppressed_ = true;
    if (channelFilter_ == QStringLiteral("Email")) {
        filteredContactModel_.setRequiredNonEmptyRole(ContactListModel::EmailRole);
    } else if (channelFilter_ == QStringLiteral("Telegram")) {
        filteredContactModel_.setRequiredNonEmptyRole(ContactListModel::TelegramRole);
    } else if (channelFilter_ == QStringLiteral("LinkedIn")) {
        filteredContactModel_.setRequiredNonEmptyRole(ContactListModel::LinkedinRole);
    } else {
        filteredContactModel_.clearRequiredNonEmptyRole();
    }
    visibleCountNotificationsSuppressed_ = false;
    selectionTracker_.endModelUpdate();
    handleVisibleCountChanged();
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
    if (searchText_.isEmpty() && companyFilter_.isEmpty() && channelFilter_.isEmpty()) {
        return;
    }

    const bool didSearchTextChange = !searchText_.isEmpty();
    const bool didCompanyFilterChange = !companyFilter_.isEmpty();
    const bool didChannelFilterChange = !channelFilter_.isEmpty();
    searchText_.clear();
    companyFilter_.clear();
    channelFilter_.clear();
    selectionTracker_.beginModelUpdate();
    visibleCountNotificationsSuppressed_ = true;
    filteredContactModel_.setSearchText(QString());
    filteredContactModel_.clearExactFilter();
    filteredContactModel_.clearRequiredNonEmptyRole();
    visibleCountNotificationsSuppressed_ = false;
    selectionTracker_.endModelUpdate();
    handleVisibleCountChanged();
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

void ContactDirectoryController::handleVisibleCountChanged()
{
    if (visibleCountNotificationsSuppressed_) {
        return;
    }

    const auto count = contactCount();
    if (publishedContactCount_ == count) {
        return;
    }

    publishedContactCount_ = count;
    emit contactCountChanged();
    emit resultSummaryChanged();
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

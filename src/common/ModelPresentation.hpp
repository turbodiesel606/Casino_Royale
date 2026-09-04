#ifndef JOBTRACKER_SRC_COMMON_MODELPRESENTATION_HPP
#define JOBTRACKER_SRC_COMMON_MODELPRESENTATION_HPP

#include <QString>

class QDate;
class QDateTime;

namespace common::presentation {

/** Returns an invariant-locale `MMM d, yyyy` label or an empty string. */
QString shortDateLabel(const QDate& date);

/** Converts to local time and returns its invariant-locale date label. */
QString shortLocalDateLabel(const QDateTime& dateTime);

/** Returns the first two characters in upper case. */
QString twoCharacterInitials(const QString& value);

/** Returns an English count label using the supplied singular and plural. */
QString countLabel(
    int count,
    const QString& singular,
    const QString& plural);

/** Returns the shared accent used for company identities. */
const QString& companyAccent();

} // namespace common::presentation

#endif // JOBTRACKER_SRC_COMMON_MODELPRESENTATION_HPP

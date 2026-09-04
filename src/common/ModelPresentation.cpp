#include "ModelPresentation.hpp"

#include <QDate>
#include <QDateTime>
#include <QLocale>

namespace common::presentation {

namespace {

const QString shortDateFormat{QStringLiteral("MMM d, yyyy")};

} // namespace

QString shortDateLabel(const QDate& date)
{
    return date.isValid()
        ? QLocale::c().toString(date, shortDateFormat)
        : QString{};
}

QString shortLocalDateLabel(const QDateTime& dateTime)
{
    return dateTime.isValid()
        ? shortDateLabel(dateTime.toLocalTime().date())
        : QString{};
}

QString twoCharacterInitials(const QString& value)
{
    return value.left(2).toUpper();
}

QString countLabel(
    int count,
    const QString& singular,
    const QString& plural)
{
    return count == 1
        ? QStringLiteral("1 %1").arg(singular)
        : QStringLiteral("%1 %2").arg(count).arg(plural);
}

const QString& companyAccent()
{
    static const QString accent{QStringLiteral("#146ce0")};
    return accent;
}

} // namespace common::presentation

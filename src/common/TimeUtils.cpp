#include "TimeUtils.hpp"

#include <QDateTime>

namespace common {

QDateTime currentUtcSecond()
{
    return QDateTime::fromString(
               QDateTime::currentDateTimeUtc().toString(Qt::ISODate),
               Qt::ISODate)
        .toUTC();
}

} // namespace common

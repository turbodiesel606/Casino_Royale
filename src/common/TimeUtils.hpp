#ifndef JOBTRACKER_SRC_COMMON_TIMEUTILS_HPP
#define JOBTRACKER_SRC_COMMON_TIMEUTILS_HPP

#include <QDateTime>

namespace common {

/** Returns the current UTC time rounded to the persistence second boundary. */
QDateTime currentUtcSecond();

} // namespace common

#endif // JOBTRACKER_SRC_COMMON_TIMEUTILS_HPP

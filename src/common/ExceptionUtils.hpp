#ifndef JOBTRACKER_SRC_COMMON_EXCEPTIONUTILS_HPP
#define JOBTRACKER_SRC_COMMON_EXCEPTIONUTILS_HPP

#include <QString>

#include <exception>

namespace common {

	/**
	 * Translates an exception into a UI-safe message without allowing the
	 * exception to cross a Qt signal, queued callback, or QML boundary.
	 */
	QString exceptionMessage(
		const std::exception_ptr& exception,
		const QString& fallbackMessage);

} // namespace common

#endif // JOBTRACKER_SRC_COMMON_EXCEPTIONUTILS_HPP

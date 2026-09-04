#include "ExceptionUtils.hpp"

#include <exception>

namespace common {

	QString exceptionMessage(
		const std::exception_ptr& exception,
		const QString& fallbackMessage)
	{
		try {
			if (exception != nullptr)
				std::rethrow_exception(exception);
		}
		catch (const std::exception& error) {
			const auto message = QString::fromUtf8(error.what());
			if (!message.isEmpty()) 
				return message;
		}
		catch (...) {
		}

		return fallbackMessage;
	}

} // namespace common

#include "utils/Utils.hpp"
#include <QSqlError>
#include <stdexcept>

namespace utils {

	[[noreturn]] void throwQueryError(const QSqlQuery& query)
	{
		throw std::runtime_error(query.lastError().text().toStdString());
	}

	 void executeQuery(QSqlDatabase& database, const QString& statement)
	{
		QSqlQuery query(database); // create a Query object for this database connection
		if (!query.exec(statement))
			throwQueryError(query);
	}
}

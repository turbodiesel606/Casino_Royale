#include "SqlQuery.hpp"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

#include <stdexcept>

namespace {

	[[noreturn]] void throwSqlError(const QString& operationContext, const QString& errorText)
	{
		throw std::runtime_error(
			QStringLiteral("Failed to %1: %2")
			.arg(operationContext, errorText)
			.toStdString());
	}

} // namespace

namespace storage::sql {

	void throwDatabaseError(const QSqlDatabase& database, const QString& operationContext)
	{
		throwSqlError(operationContext, database.lastError().text());
	}

	void throwQueryError(const QSqlQuery& query, const QString& operationContext)
	{
		throwSqlError(operationContext, query.lastError().text());
	}

	void execute(
		QSqlDatabase& database,
		const QString& statement,
		const QString& operationContext)
	{
		QSqlQuery query{ database };
		if (!query.exec(statement)) {
			throwQueryError(query, operationContext);
		}
	}

	void execute(QSqlQuery& query, const QString& operationContext)
	{
		if (!query.exec()) {
			throwQueryError(query, operationContext);
		}
	}

	QString nonNullText(const QString& value)
	{
		return value.isNull() ? QStringLiteral("") : value;
	}

	QDateTime readIsoDateTime(const QSqlQuery& query, const QString& columnName)
	{
		return QDateTime::fromString(query.value(columnName).toString(), Qt::ISODate);
	}

} // namespace storage::sql

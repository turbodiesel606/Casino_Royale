#include "SqlQuery.hpp"
#include "logging/ApplicationLog.hpp"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

#include <exception>
#include <stdexcept>

namespace {

	[[noreturn]] void throwSqlError(const QString& operationContext, const QSqlError& error)
	{
		throw storage::sql::Failure(
			QStringLiteral("Failed to %1: %2")
			.arg(operationContext, error.text())
			.toStdString(), storage::sql::failureCause(error));
	}

} // namespace

namespace storage::sql {

	Failure::Failure(const std::string& message, LogFact cause)
		: std::runtime_error{message}, cause_{cause}
	{
	}

	LogFact Failure::cause() const noexcept
	{
		return cause_;
	}

	LogFact failureCause(const QSqlError& error) noexcept
	{
		// Codex Comment: SQLite extended codes retain the primary result in their low byte.
		switch (error.nativeErrorCode().toInt() & 0xff) {
		case 3: return LogFact::PermissionDenied;
		case 5:
		case 6: return LogFact::DbBusy;
		case 7: return LogFact::DbNoMemory;
		case 8: return LogFact::DbReadOnly;
		case 10: return LogFact::DbIo;
		case 11:
		case 26: return LogFact::DbCorrupt;
		case 13: return LogFact::DbFull;
		case 14: return LogFact::DbCannotOpen;
		case 19: return LogFact::DbConstraint;
		default: return LogFact::DatabaseError;
		}
	}

    LogFact cursorFailure(const QSqlQuery& query) noexcept
    {
        const auto error = query.lastError();
        return error.isValid() ? failureCause(error) : LogFact::None;
    }

    void finishDatasetRead(const QSqlQuery& query, LogFact dataset, bool* allReadsComplete) noexcept
    {
        const auto cause = cursorFailure(query);
        if (cause != LogFact::None) {
            if (allReadsComplete)
                *allReadsComplete = false;
            ApplicationLog::record(LogEventType::SavedDatasetLoadFailed, dataset, cause);
        }
    }

	LogFact currentFailureCause() noexcept
	{
		const auto error = std::current_exception();
		if (error) {
			try { std::rethrow_exception(error); }
			catch (const Failure& failure) { return failure.cause(); }
			catch (const std::bad_alloc&) { return LogFact::DbNoMemory; }
			catch (...) {}
		}
		return LogFact::CauseUnavailable;
	}

	void throwDatabaseError(const QSqlDatabase& database, const QString& operationContext)
	{
		throwSqlError(operationContext, database.lastError());
	}

	void throwQueryError(const QSqlQuery& query, const QString& operationContext)
	{
		throwSqlError(operationContext, query.lastError());
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
		if (!query.exec())
			throwQueryError(query, operationContext);
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

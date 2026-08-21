#include "SchemaMigrator.hpp"

#include "SchemaMigrationSteps.hpp"
#include "SqlQuery.hpp"
#include "SqlTransaction.hpp"

#include <QSqlDatabase>
#include <QSqlQuery>

#include <stdexcept>

namespace {

	constexpr int latestSchemaVersion = 4;

	int schemaVersion(QSqlDatabase& database)
	{   // Read the current SQLite schema version from PRAGMA user version.
		QSqlQuery query{ database };
		if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) {
			storage::sql::throwQueryError(
				query,
				QStringLiteral("read the SQLite schema version"));
		}
		return query.value(0).toInt();
	}

	void verifyForeignKeys(QSqlDatabase& database)
	{

		QSqlQuery query{ database };
		if (!query.exec(QStringLiteral("PRAGMA foreign_key_check"))) {
			storage::sql::throwQueryError(
				query,
				QStringLiteral("verify schema migration foreign keys"));
		}
		if (query.next()) { // check if next row contain invalid foreign-key relationships.
			throw std::runtime_error(
				"The JobTracker database contains invalid foreign-key relationships.");
		}
	}

}

void SchemaMigrator::migrate(QSqlDatabase& database)
{
	int version = schemaVersion(database);

	// No migration is needed when the database already uses the latest schema version.
	if (version == latestSchemaVersion)
		return;

	// Reject databases created by a newer application version to avoid unsafe schema changes.
	if (version > latestSchemaVersion)
		throw std::runtime_error(
			"The JobTracker database schema is newer than this application supports.");

	// Start one transaction so schema changes are committed or rolled back as a unit.
	SqlTransaction transaction{ database, QStringLiteral("schema migration") };

	// Initialize a new database directly at the latest schema version.
	if (version == 0) {
		storage::migrations::initializeVersionFour(database);
		version = latestSchemaVersion;
	}


	// Applies each required forward migration so older databases reach the latest schema version.
	// Change schema updating logic in future. 
	// For example: migrate directly from version 1 to version 3, 4, 5, etc, 
	// rather than first to version 2 and then to version 3, and then etc...
	while (version < latestSchemaVersion) {
		switch (version) {
		case 1:
			// Upgrades a version 1 database to version 2 with preserving existing data.
			storage::migrations::migrateVersionOneToTwo(database);
			version = 2;
			break;
		case 2:
			// Upgrades a version 2 database to version 3 with preserving existing data.
			storage::migrations::migrateVersionTwoToThree(database);
			version = 3;
			break;
		case 3:
			// Adds reversible CV Library archival state without changing file identity.
			storage::migrations::migrateVersionThreeToFour(database);
			version = 4;
			break;
		default:
			throw std::runtime_error(
				"The JobTracker database schema version is unsupported.");
		}
	}

	// Verifies that schema initialization or migration produced no invalid foreign-key relationships.
	verifyForeignKeys(database);
	transaction.commit();
}

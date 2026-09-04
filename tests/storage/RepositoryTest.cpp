#include "cvs/CvRepository.hpp"
#include "storage/SqlQuery.hpp"
#include "storage/SqlTransaction.hpp"

#include "../support/StorageTestFixtures.hpp"

#include <QDateTime>
#include <QSqlQuery>
#include <QtTest/QtTest>

#include <exception>

class RepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void queryErrorsIncludeOperationContext();
    void preparedQueryErrorsIncludeOperationContext();
    void sqlLeafConversionsPreserveValues();
    void transactionGuardRollsBackUntilCommitted();
    void persistsFavoriteAcrossDatabaseReopen();
};

void RepositoryTest::queryErrorsIncludeOperationContext()
{
    testsupport::TemporaryDatabaseFixture fixture;
    QVERIFY(fixture.isValid());

    QString errorMessage;
    try {
        storage::sql::execute(
            fixture.database().connection(),
            QStringLiteral("INSERT INTO missing_table VALUES (1)"),
            QStringLiteral("insert a query-helper test row"));
    } catch (const std::exception& error) {
        errorMessage = QString::fromUtf8(error.what());
    }

    QVERIFY(errorMessage.contains(QStringLiteral("insert a query-helper test row")));
    QVERIFY(errorMessage.contains(QStringLiteral("missing_table")));
}

void RepositoryTest::preparedQueryErrorsIncludeOperationContext()
{
    testsupport::TemporaryDatabaseFixture fixture;
    QVERIFY(fixture.isValid());

    auto& connection = fixture.database().connection();
    storage::sql::execute(
        connection,
        QStringLiteral("CREATE TABLE prepared_query_probe (value INTEGER PRIMARY KEY)"),
        QStringLiteral("create the prepared query probe"));
    storage::sql::execute(
        connection,
        QStringLiteral("INSERT INTO prepared_query_probe VALUES (1)"),
        QStringLiteral("insert the first prepared query probe row"));

    QSqlQuery query{connection};
    QVERIFY(query.prepare(QStringLiteral(
        "INSERT INTO prepared_query_probe VALUES (:value)")));
    query.bindValue(QStringLiteral(":value"), 1);

    QString errorMessage;
    try {
        storage::sql::execute(
            query,
            QStringLiteral("execute a prepared query-helper test statement"));
    } catch (const std::exception& error) {
        errorMessage = QString::fromUtf8(error.what());
    }

    QVERIFY(errorMessage.contains(QStringLiteral("prepared query-helper test statement")));
    QVERIFY(errorMessage.contains(QStringLiteral("UNIQUE"), Qt::CaseInsensitive));
}

void RepositoryTest::sqlLeafConversionsPreserveValues()
{
    QString nullText;
    QVERIFY(nullText.isNull());
    const auto normalizedNull = storage::sql::nonNullText(nullText);
    QVERIFY(!normalizedNull.isNull());
    QVERIFY(normalizedNull.isEmpty());

    const auto text = QStringLiteral("  unchanged  ");
    QCOMPARE(storage::sql::nonNullText(text), text);

    testsupport::TemporaryDatabaseFixture fixture;
    QVERIFY(fixture.isValid());
    auto& connection = fixture.database().connection();
    storage::sql::execute(
        connection,
        QStringLiteral("CREATE TABLE timestamp_probe (value TEXT)"),
        QStringLiteral("create the timestamp conversion probe"));
    storage::sql::execute(
        connection,
        QStringLiteral("INSERT INTO timestamp_probe VALUES "
                       "('2026-08-24T10:20:30Z'), (NULL)"),
        QStringLiteral("insert timestamp conversion probes"));

    QSqlQuery query{connection};
    QVERIFY(query.exec(QStringLiteral("SELECT value FROM timestamp_probe ORDER BY rowid")));
    QVERIFY(query.next());
    QCOMPARE(
        storage::sql::readIsoDateTime(query, QStringLiteral("value")),
        QDateTime::fromString(QStringLiteral("2026-08-24T10:20:30Z"), Qt::ISODate));
    QVERIFY(query.next());
    QVERIFY(!storage::sql::readIsoDateTime(query, QStringLiteral("value")).isValid());
}

void RepositoryTest::transactionGuardRollsBackUntilCommitted()
{
    testsupport::TemporaryDatabaseFixture fixture;
    QVERIFY(fixture.isValid());
    auto& connection = fixture.database().connection();
    storage::sql::execute(
        connection,
        QStringLiteral("CREATE TABLE transaction_probe (value INTEGER NOT NULL)"),
        QStringLiteral("create the transaction-guard probe table"));

    {
        SqlTransaction transaction{connection, QStringLiteral("transaction-guard rollback test")};
        storage::sql::execute(
            connection,
            QStringLiteral("INSERT INTO transaction_probe VALUES (1)"),
            QStringLiteral("insert the rollback probe row"));
    }

    QSqlQuery countAfterRollback{connection};
    QVERIFY(countAfterRollback.exec(QStringLiteral("SELECT COUNT(*) FROM transaction_probe")));
    QVERIFY(countAfterRollback.next());
    QCOMPARE(countAfterRollback.value(0).toInt(), 0);

    {
        SqlTransaction transaction{connection, QStringLiteral("transaction-guard commit test")};
        storage::sql::execute(
            connection,
            QStringLiteral("INSERT INTO transaction_probe VALUES (2)"),
            QStringLiteral("insert the commit probe row"));
        transaction.commit();
    }

    QSqlQuery countAfterCommit{connection};
    QVERIFY(countAfterCommit.exec(QStringLiteral("SELECT COUNT(*) FROM transaction_probe")));
    QVERIFY(countAfterCommit.next());
    QCOMPARE(countAfterCommit.value(0).toInt(), 1);
}

void RepositoryTest::persistsFavoriteAcrossDatabaseReopen()
{
    testsupport::TemporaryStorageFixture storage;
    QVERIFY(storage.isValid());
    const auto originalUpdatedAt = QDateTime::fromString(
        QStringLiteral("2026-07-01T10:00:00Z"),
        Qt::ISODate);

    {
        SqliteDatabase database{storage.paths().databasePath()};
        CvRepository cvs{database.connection()};
        CvDocument document;
        document.id_ = QStringLiteral("cv-favorite");
        document.originalFileName_ = QStringLiteral("favorite.pdf");
        document.storedFileName_ = QStringLiteral("stored-favorite.pdf");
        document.relativePath_ = QStringLiteral("Resumes/stored-favorite.pdf");
        document.sha256_ = QStringLiteral("favorite-hash");
        document.sizeBytes_ = 42;
        document.createdAt_ = originalUpdatedAt;
        document.updatedAt_ = originalUpdatedAt;
        cvs.insert(document);

        const auto updatedAt = cvs.updateFavorite(document.id_, true);
        QVERIFY(updatedAt.has_value());
        QVERIFY(*updatedAt != originalUpdatedAt);
    }

    SqliteDatabase reopened{storage.paths().databasePath()};
    CvRepository cvs{reopened.connection()};
    const auto persisted = cvs.findAll();
    QCOMPARE(persisted.size(), 1);
    QCOMPARE(persisted.first().id_, QStringLiteral("cv-favorite"));
    QVERIFY(persisted.first().isFavorite_);
    QVERIFY(persisted.first().updatedAt_ != originalUpdatedAt);
}

QTEST_GUILESS_MAIN(RepositoryTest)

#include "RepositoryTest.moc"

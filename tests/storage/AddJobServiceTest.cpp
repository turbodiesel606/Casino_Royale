#include "../support/AddJobTestFixture.hpp"
#include "common/CancellationState.hpp"
#include "jobs/JobApplicationValidator.hpp"
#include "storage/SqlTransaction.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QUrl>
#include <QtTest/QtTest>

#include <memory>
#include <future>
#include <thread>
#include <stdexcept>
#include <utility>

class AddJobServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsJobAndCopiesCv();
    void restoresArchivedCv();
    void rollsBackArchivedRestorationWhenJobInsertFails();
    void removesFinalizedCvWhenCvInsertFails();
    void reusesNormalizedCompanyIdentity();
    void removesCopiedCvWhenJobInsertFails();
    void rejectsInvalidInputWithoutWriting();
    void preflightNormalizesAndReturnsAllValidationErrors();
    void prepareDefensivelyRejectsInvalidDraftWithoutStaging();
    void cancelsPreparedJobBeforeTransaction();
    void cancellationAfterFinalRenameStillCommits();
};

void AddJobServiceTest::createsJobAndCopiesCv()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto sourcePath = fixture.storage_.createFile();

    const auto result = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(sourcePath));

    QVERIFY2(result.success_, qPrintable(result.message_));
    QCOMPARE(result.cvImportDisposition_, CvImportDisposition::Inserted);
    QVERIFY(!result.company_.id_.isEmpty());
    QCOMPARE(result.application_.companyId_, result.company_.id_);

    const auto applications = fixture.jobRepository_.findAll();
    const auto cvs = fixture.cvRepository_.findAll();
    const auto companies = fixture.companyRepository_.findAll();
    QCOMPARE(applications.size(), 1);
    QCOMPARE(
        applications.first().techStack_,
        QStringList({QStringLiteral("Qt"), QStringLiteral("C++")}));
    QCOMPARE(
        applications.first().jobUrl_,
        QUrl{QStringLiteral("https://example.com/jobs/qt")});
    QCOMPARE(applications.first().workFormat_, WorkFormat::Remote);
    QCOMPARE(applications.first().status_, JobStatus::Applied);
    QCOMPARE(applications.first().appliedDate_, QDate(2026, 7, 9));
    QVERIFY(applications.first().createdAt_.isValid());
    QCOMPARE(applications.first().createdAt_, applications.first().updatedAt_);
    QCOMPARE(applications.first().createdAt_.time().msec(), 0);
    QCOMPARE(cvs.size(), 1);
    QVERIFY(cvs.first().createdAt_.isValid());
    QCOMPARE(companies.size(), 1);
    QVERIFY(companies.first().createdAt_.isValid());
    QVERIFY(QFileInfo::exists(
        QDir{fixture.storage_.paths().dataDirectory()}.filePath(result.cvDocument_.relativePath_)));

    QSqlQuery invalidCompany{fixture.database_.connection()};
    invalidCompany.prepare(QStringLiteral("UPDATE jobs SET company_id = ? WHERE id = ?"));
    invalidCompany.addBindValue(QStringLiteral("missing-company"));
    invalidCompany.addBindValue(result.application_.id_);
    QVERIFY(!invalidCompany.exec());
}

void AddJobServiceTest::restoresArchivedCv()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto sourcePath = fixture.storage_.createFile(QStringLiteral("archived.pdf"));
    const auto first = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(sourcePath));
    QVERIFY(first.success_);
    QVERIFY(fixture.cvRepository_.updateArchived(first.cvDocument_.id_, true).has_value());

    auto secondDraft = testsupport::validJobDraft();
    secondDraft.jobTitle_ = QStringLiteral("Second Qt Role");
    const auto second = fixture.service_.create(secondDraft, QUrl::fromLocalFile(sourcePath));

    QVERIFY2(second.success_, qPrintable(second.message_));
    QCOMPARE(second.cvImportDisposition_, CvImportDisposition::RestoredArchived);
    QCOMPARE(second.cvDocument_.id_, first.cvDocument_.id_);
    QCOMPARE(fixture.cvRepository_.findAll().size(), 1);
    QVERIFY(!fixture.cvRepository_.findById(first.cvDocument_.id_)->archivedAt_.isValid());
    QCOMPARE(fixture.jobRepository_.findAll().size(), 2);
    QCOMPARE(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).size(), 1);
}

void AddJobServiceTest::rollsBackArchivedRestorationWhenJobInsertFails()
{
    testsupport::AddJobTestFixture fixture;
    const auto source = QUrl::fromLocalFile(fixture.storage_.createFile());
    const auto first = fixture.service_.create(testsupport::validJobDraft(), source);
    QVERIFY(first.success_);
    QVERIFY(fixture.cvRepository_.updateArchived(first.cvDocument_.id_, true).has_value());
    const auto archived = *fixture.cvRepository_.findById(first.cvDocument_.id_);
    QSqlQuery trigger{fixture.database_.connection()};
    QVERIFY(trigger.exec(QStringLiteral(
        "CREATE TRIGGER reject_job BEFORE INSERT ON jobs "
        "BEGIN SELECT RAISE(FAIL, 'forced job failure'); END")));

    auto draft = testsupport::validJobDraft();
    draft.companyName_ = QStringLiteral("Rolled Back Company");
    const auto failed = fixture.service_.create(draft, source);
    QVERIFY(!failed.success_);
    const auto stored = *fixture.cvRepository_.findById(archived.id_);
    QCOMPARE(stored.archivedAt_, archived.archivedAt_);
    QCOMPARE(stored.updatedAt_, archived.updatedAt_);
    QCOMPARE(fixture.companyRepository_.findAll().size(), 1);
    QCOMPARE(fixture.jobRepository_.findAll().size(), 1);
    QCOMPARE(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).size(), 1);

    QVERIFY(trigger.exec(QStringLiteral("DROP TRIGGER reject_job")));
    const auto retry = fixture.service_.create(draft, source);
    QVERIFY2(retry.success_, qPrintable(retry.message_));
    QCOMPARE(retry.cvImportDisposition_, CvImportDisposition::RestoredArchived);
}

void AddJobServiceTest::removesFinalizedCvWhenCvInsertFails()
{
    testsupport::AddJobTestFixture fixture;
    QSqlQuery trigger{fixture.database_.connection()};
    QVERIFY(trigger.exec(QStringLiteral(
        "CREATE TRIGGER reject_cv BEFORE INSERT ON cvs "
        "BEGIN SELECT RAISE(FAIL, 'forced CV failure'); END")));
    const auto result = fixture.service_.create(
        testsupport::validJobDraft(), QUrl::fromLocalFile(fixture.storage_.createFile()));
    QVERIFY(!result.success_);
    QVERIFY(result.message_.contains(QStringLiteral("forced CV failure")));
    QVERIFY(fixture.cvRepository_.findAll().isEmpty());
    QVERIFY(fixture.companyRepository_.findAll().isEmpty());
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QVERIFY(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).isEmpty());
}

void AddJobServiceTest::reusesNormalizedCompanyIdentity()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto sourcePath = fixture.storage_.createFile();

    const auto first = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(sourcePath));
    auto secondDraft = testsupport::validJobDraft();
    secondDraft.jobTitle_ = QStringLiteral("Senior Qt Developer");
    secondDraft.companyName_ = QStringLiteral("  example COMPANY  ");
    const auto second = fixture.service_.create(
        secondDraft,
        QUrl::fromLocalFile(sourcePath));

    QVERIFY(first.success_);
    QVERIFY(second.success_);
    QCOMPARE(first.application_.companyId_, second.application_.companyId_);
    QCOMPARE(second.application_.companyName_, QStringLiteral("Example Company"));
    QCOMPARE(fixture.companyRepository_.findAll().size(), 1);
    QCOMPARE(fixture.cvRepository_.findAll().size(), 1);
    QCOMPARE(fixture.jobRepository_.findAll().size(), 2);
}

void AddJobServiceTest::removesCopiedCvWhenJobInsertFails()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    QSqlQuery trigger{fixture.database_.connection()};
    QVERIFY(trigger.exec(QStringLiteral(
        "CREATE TRIGGER reject_job BEFORE INSERT ON jobs "
        "BEGIN SELECT RAISE(FAIL, 'forced job failure'); END")));

    const auto result = fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(fixture.storage_.createFile()));

    QVERIFY(!result.success_);
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QVERIFY(fixture.companyRepository_.findAll().isEmpty());
    QVERIFY(fixture.cvRepository_.findAll().isEmpty());
    QVERIFY(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).isEmpty());
}

void AddJobServiceTest::rejectsInvalidInputWithoutWriting()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    auto draft = testsupport::validJobDraft();
    draft.jobTitle_.clear();
    draft.jobUrl_ = QStringLiteral("file:///not-a-job");

    const auto result = fixture.service_.create(draft, {});

    QVERIFY(!result.success_);
    QVERIFY(result.fieldErrors_.contains(QStringLiteral("jobTitle")));
    QVERIFY(result.fieldErrors_.contains(QStringLiteral("jobUrl")));
    QVERIFY(result.fieldErrors_.contains(QStringLiteral("cv")));
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QVERIFY(fixture.companyRepository_.findAll().isEmpty());
    QVERIFY(fixture.cvRepository_.findAll().isEmpty());
    QVERIFY(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).isEmpty());
}

void AddJobServiceTest::preflightNormalizesAndReturnsAllValidationErrors()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    auto validDraft = testsupport::validJobDraft();
    validDraft.jobTitle_ = QStringLiteral("  Qt Developer  ");
    validDraft.jobUrl_.clear();

    const auto validPreflight = JobApplicationValidator::preflight(validDraft, true);

    QVERIFY2(validPreflight.isValid(), qPrintable(validPreflight.message_));
    QCOMPARE(validPreflight.draft_.jobTitle_, QStringLiteral("Qt Developer"));
    QVERIFY(validPreflight.draft_.jobUrl_.isEmpty());
    QVERIFY(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).isEmpty());

    auto invalidDraft = testsupport::validJobDraft();
    invalidDraft.jobTitle_ = QStringLiteral(" ");
    invalidDraft.companyName_.clear();
    invalidDraft.jobUrl_ = QStringLiteral("ftp://example.com/job");
    invalidDraft.workFormat_ = QStringLiteral("Office");
    invalidDraft.status_ = QStringLiteral("Pending");
    invalidDraft.appliedDate_ = QStringLiteral("2026-99-87");

    const auto invalidPreflight = JobApplicationValidator::preflight(invalidDraft, false);

    QVERIFY(!invalidPreflight.isValid());
    const QStringList expectedFields{
        QStringLiteral("jobTitle"),
        QStringLiteral("companyName"),
        QStringLiteral("jobUrl"),
        QStringLiteral("workFormat"),
        QStringLiteral("status"),
        QStringLiteral("appliedDate"),
        QStringLiteral("cv"),
    };
    for (const auto& field : expectedFields) {
        QVERIFY2(invalidPreflight.fieldErrors_.contains(field), qPrintable(field));
    }
    QVERIFY(!invalidPreflight.message_.isEmpty());
}

void AddJobServiceTest::prepareDefensivelyRejectsInvalidDraftWithoutStaging()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    auto draft = testsupport::validJobDraft();
    draft.companyName_.clear();
    const auto sourceUrl = QUrl::fromLocalFile(fixture.storage_.createFile());
    const auto preflight = JobApplicationValidator::preflight(draft, true);
    QVERIFY(!preflight.isValid());
    const auto cancellation = std::make_shared<CancellationState>();

    const auto preparation = fixture.service_.prepare(
        preflight.draft_,
        sourceUrl,
        cancellation);

    QVERIFY(!preparation.success_);
    QVERIFY(preparation.fieldErrors_.contains(QStringLiteral("companyName")));
    QVERIFY(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).isEmpty());
}

void AddJobServiceTest::cancelsPreparedJobBeforeTransaction()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto sourceUrl = QUrl::fromLocalFile(fixture.storage_.createFile());
    const auto cancellation = std::make_shared<CancellationState>();
    const auto preflight = JobApplicationValidator::preflight(
        testsupport::validJobDraft(),
        true);
    QVERIFY(preflight.isValid());

    auto preparation = fixture.service_.prepare(
        preflight.draft_,
        sourceUrl,
        cancellation);
    QVERIFY2(preparation.success_, qPrintable(preparation.message_));
    cancellation->requestCancellation();

    const auto result = fixture.service_.complete(
        std::move(preparation),
        cancellation);

    QVERIFY(!result.success_);
    QVERIFY(result.message_.contains(QStringLiteral("canceled"), Qt::CaseInsensitive));
    QVERIFY(fixture.jobRepository_.findAll().isEmpty());
    QVERIFY(fixture.companyRepository_.findAll().isEmpty());
    QVERIFY(fixture.cvRepository_.findAll().isEmpty());
    QVERIFY(QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).isEmpty());
}

void AddJobServiceTest::cancellationAfterFinalRenameStillCommits()
{
    testsupport::AddJobTestFixture fixture;
    const auto source = QUrl::fromLocalFile(fixture.storage_.createFile());
    const auto cancellation = std::make_shared<CancellationState>();
    const auto draft = JobApplicationValidator::preflight(testsupport::validJobDraft(), true);
    auto preparation = fixture.service_.prepare(draft.draft_, source, cancellation);
    QVERIFY(preparation.success_);

    std::promise<void> readerReady;
    auto ready = readerReady.get_future();
    const auto databasePath = fixture.storage_.paths().databasePath();
    const auto directory = fixture.storage_.paths().resumesDirectory();
    bool sawFinalFile = false;
    std::jthread reader{[&](std::stop_token stop) {
        try {
            SqliteDatabase database{databasePath};
            SqlTransaction transaction{database.connection(), QStringLiteral("Hold commit for cancellation")};
            QSqlQuery query{database.connection()};
            if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM cvs")) || !query.next()) {
                throw std::runtime_error("Could not establish the reader lock.");
            }
            readerReady.set_value();
            while (!stop.stop_requested()) {
                const auto files = QDir{directory}.entryList({QStringLiteral("*.pdf")}, QDir::Files);
                if (!files.isEmpty()) {
                    sawFinalFile = true;
                    cancellation->requestCancellation();
                    break;
                }
                std::this_thread::yield();
            }
            query.finish();
            transaction.commit();
        } catch (...) {
            try { readerReady.set_exception(std::current_exception()); } catch (...) {}
        }
    }};
    ready.get();
    const auto result = fixture.service_.complete(std::move(preparation), cancellation);
    reader.request_stop();
    reader.join();

    QVERIFY(sawFinalFile);
    QVERIFY(cancellation->isCancellationRequested());
    QVERIFY2(result.success_, qPrintable(result.message_));
    QCOMPARE(fixture.jobRepository_.findAll().size(), 1);
    QCOMPARE(fixture.cvRepository_.findAll().size(), 1);
    QCOMPARE(QDir{directory}.entryList(QDir::Files).size(), 1);
}

QTEST_GUILESS_MAIN(AddJobServiceTest)

#include "AddJobServiceTest.moc"

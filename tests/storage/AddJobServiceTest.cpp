#include "../support/AddJobTestFixture.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QUrl>
#include <QtTest/QtTest>

class AddJobServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsJobAndCopiesCv();
    void reusesNormalizedCompanyIdentity();
    void removesCopiedCvWhenJobInsertFails();
    void rejectsInvalidInputWithoutWriting();
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
    QVERIFY(result.cvWasInserted_);
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

QTEST_GUILESS_MAIN(AddJobServiceTest)

#include "AddJobServiceTest.moc"

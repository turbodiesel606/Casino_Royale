#include "../support/AddJobTestFixture.hpp"

#include "common/CancellationState.hpp"
#include "jobs/JobApplicationValidator.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QUrl>
#include <QtTest/QtTest>

#include <memory>
#include <utility>

namespace {

UpdateJobResult updateJob(
    testsupport::AddJobTestFixture& fixture,
    const QString& applicationId,
    const JobApplicationDraft& draft,
    const QUrl& replacementCvUrl = {})
{
    const auto preflight = JobApplicationValidator::preflight(draft, true);
    if (!preflight.isValid()) {
        UpdateJobResult result;
        result.fieldErrors_ = preflight.fieldErrors_;
        result.message_ = preflight.message_;
        return result;
    }

    const auto cancellation = std::make_shared<CancellationState>();
    auto preparation = fixture.updateService_.prepare(
        applicationId,
        preflight.draft_,
        replacementCvUrl,
        cancellation);
    return fixture.updateService_.complete(
        std::move(preparation),
        cancellation);
}

AddJobResult createInitialJob(
    testsupport::AddJobTestFixture& fixture,
    const QString& fileName = QStringLiteral("original.pdf"),
    const QByteArray& contents = QByteArrayLiteral("%PDF original CV"))
{
    return fixture.service_.create(
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(fixture.storage_.createFile(fileName, contents)));
}

} // namespace

class UpdateJobServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void updatesMetadataAndOrderedTechnologiesInPlace();
    void resolvesNewCompanyAndReusesNormalizedCompany();
    void importsReplacementAndPreservesPreviousCv();
    void reusesActiveAndArchivedReplacementDuplicates();
    void rollsBackDatabaseAndFileOnUpdateFailure();
    void rejectsMissingAndInvalidApplicationsWithoutMutation();
    void cancelsPreparedReplacementBeforeTransaction();
    void persistsUpdateAcrossDatabaseReopen();
};

void UpdateJobServiceTest::updatesMetadataAndOrderedTechnologiesInPlace()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto original = createInitialJob(fixture);
    QVERIFY2(original.success_, qPrintable(original.message_));

    auto draft = testsupport::validJobDraft();
    draft.jobTitle_ = QStringLiteral("  Senior Qt Engineer  ");
    draft.jobUrl_ = QStringLiteral(" https://example.com/jobs/senior-qt ");
    draft.workFormat_ = QStringLiteral("Hybrid");
    draft.city_ = QStringLiteral(" Baku ");
    draft.salary_ = QStringLiteral(" 7000 ");
    draft.status_ = QStringLiteral("Interview");
    draft.appliedDate_ = QStringLiteral("2026-08-20");
    draft.nextStep_ = QStringLiteral(" Architecture interview ");
    draft.description_ = QStringLiteral(" Build desktop products ");
    draft.requirements_ = QStringLiteral(" Modern C++ and Qt ");
    draft.techStack_ = {
        QStringLiteral(" Qt "),
        QStringLiteral("C++"),
        QStringLiteral("qt"),
        QStringLiteral(" QML ")};
    draft.notes_ = QStringLiteral(" Follow up next week ");

    const auto result = updateJob(fixture, original.application_.id_, draft);

    QVERIFY2(result.success_, qPrintable(result.message_));
    QCOMPARE(result.application_.id_, original.application_.id_);
    QCOMPARE(result.application_.createdAt_, original.application_.createdAt_);
    QCOMPARE(result.application_.cvId_, original.application_.cvId_);
    QCOMPARE(result.application_.jobTitle_, QStringLiteral("Senior Qt Engineer"));
    QCOMPARE(result.application_.techStack_, QStringList({
        QStringLiteral("Qt"),
        QStringLiteral("C++"),
        QStringLiteral("QML")}));
    QVERIFY(result.application_.updatedAt_ > original.application_.updatedAt_);

    const auto stored = fixture.jobRepository_.findById(original.application_.id_);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->jobTitle_, result.application_.jobTitle_);
    QCOMPARE(stored->jobUrl_, QUrl{QStringLiteral("https://example.com/jobs/senior-qt")});
    QCOMPARE(stored->workFormat_, WorkFormat::Hybrid);
    QCOMPARE(stored->city_, QStringLiteral("Baku"));
    QCOMPARE(stored->salary_, QStringLiteral("7000"));
    QCOMPARE(stored->status_, JobStatus::Interview);
    QCOMPARE(stored->appliedDate_, QDate(2026, 8, 20));
    QCOMPARE(stored->nextStep_, QStringLiteral("Architecture interview"));
    QCOMPARE(stored->techStack_, result.application_.techStack_);
}

void UpdateJobServiceTest::resolvesNewCompanyAndReusesNormalizedCompany()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto original = createInitialJob(fixture);
    QVERIFY(original.success_);

    auto draft = testsupport::validJobDraft();
    draft.companyName_ = QStringLiteral("  New Company  ");
    const auto moved = updateJob(fixture, original.application_.id_, draft);
    QVERIFY2(moved.success_, qPrintable(moved.message_));
    QVERIFY(moved.company_.id_ != original.company_.id_);
    QCOMPARE(fixture.companyRepository_.findAll().size(), 2);

    draft.companyName_ = QStringLiteral("  example COMPANY  ");
    const auto movedBack = updateJob(fixture, original.application_.id_, draft);
    QVERIFY2(movedBack.success_, qPrintable(movedBack.message_));
    QCOMPARE(movedBack.company_.id_, original.company_.id_);
    QCOMPARE(movedBack.application_.companyName_, QStringLiteral("Example Company"));
    QCOMPARE(fixture.companyRepository_.findAll().size(), 2);
}

void UpdateJobServiceTest::importsReplacementAndPreservesPreviousCv()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto original = createInitialJob(fixture);
    QVERIFY(original.success_);
    const auto replacementSource = fixture.storage_.createFile(
        QStringLiteral("replacement.docx"),
        QByteArrayLiteral("replacement CV contents"));

    const auto result = updateJob(
        fixture,
        original.application_.id_,
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(replacementSource));

    QVERIFY2(result.success_, qPrintable(result.message_));
    QVERIFY(result.replacementCvDocument_.has_value());
    QCOMPARE(result.cvImportDisposition_, CvImportDisposition::Inserted);
    QCOMPARE(result.previousCvId_, original.cvDocument_.id_);
    QVERIFY(result.application_.cvId_ != original.application_.cvId_);
    QCOMPARE(fixture.cvRepository_.findAll().size(), 2);
    QVERIFY(fixture.cvRepository_.findById(original.cvDocument_.id_).has_value());
    QVERIFY(QFileInfo::exists(QDir{fixture.storage_.paths().dataDirectory()}.filePath(
        original.cvDocument_.relativePath_)));
    QCOMPARE(
        QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).size(),
        2);

    const auto previousCv = fixture.cvRepository_.findById(original.cvDocument_.id_);
    const auto replacementCv = fixture.cvRepository_.findById(result.application_.cvId_);
    QVERIFY(previousCv.has_value());
    QVERIFY(replacementCv.has_value());
    QVERIFY(!previousCv->linkedApplicationIds_.contains(original.application_.id_));
    QVERIFY(replacementCv->linkedApplicationIds_.contains(original.application_.id_));
}

void UpdateJobServiceTest::reusesActiveAndArchivedReplacementDuplicates()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto original = createInitialJob(fixture);
    QVERIFY(original.success_);

    const auto candidateSource = fixture.storage_.createFile(
        QStringLiteral("candidate.pdf"),
        QByteArrayLiteral("%PDF candidate CV"));
    auto secondDraft = testsupport::validJobDraft();
    secondDraft.jobTitle_ = QStringLiteral("Second role");
    const auto second = fixture.service_.create(
        secondDraft,
        QUrl::fromLocalFile(candidateSource));
    QVERIFY(second.success_);

    const auto activeReuse = updateJob(
        fixture,
        original.application_.id_,
        testsupport::validJobDraft(),
        QUrl::fromLocalFile(candidateSource));
    QVERIFY2(activeReuse.success_, qPrintable(activeReuse.message_));
    QCOMPARE(activeReuse.cvImportDisposition_, CvImportDisposition::ExistingActive);
    QCOMPARE(activeReuse.application_.cvId_, second.cvDocument_.id_);
    QCOMPARE(fixture.cvRepository_.findAll().size(), 2);

    QVERIFY(fixture.cvRepository_.updateArchived(original.cvDocument_.id_, true).has_value());
    const auto archivedReuse = updateJob(
        fixture,
        second.application_.id_,
        secondDraft,
        QUrl::fromLocalFile(fixture.storage_.createFile(
            QStringLiteral("original.pdf"),
            QByteArrayLiteral("%PDF original CV"))));
    QVERIFY2(archivedReuse.success_, qPrintable(archivedReuse.message_));
    QCOMPARE(archivedReuse.cvImportDisposition_, CvImportDisposition::ReusedArchived);
    QCOMPARE(archivedReuse.application_.cvId_, original.cvDocument_.id_);
    const auto archived = fixture.cvRepository_.findById(original.cvDocument_.id_);
    QVERIFY(archived.has_value());
    QVERIFY(archived->archivedAt_.isValid());
    QCOMPARE(fixture.cvRepository_.findAll().size(), 2);
}

void UpdateJobServiceTest::rollsBackDatabaseAndFileOnUpdateFailure()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto original = createInitialJob(fixture);
    QVERIFY(original.success_);

    QSqlQuery trigger{fixture.database_.connection()};
    QVERIFY(trigger.exec(QStringLiteral(
        "CREATE TRIGGER reject_job_update BEFORE UPDATE ON jobs "
        "BEGIN SELECT RAISE(FAIL, 'forced job update failure'); END")));

    auto draft = testsupport::validJobDraft();
    draft.jobTitle_ = QStringLiteral("Must Roll Back");
    const auto result = updateJob(
        fixture,
        original.application_.id_,
        draft,
        QUrl::fromLocalFile(fixture.storage_.createFile(
            QStringLiteral("failed-replacement.pdf"),
            QByteArrayLiteral("%PDF failed replacement"))));

    QVERIFY(!result.success_);
    const auto stored = fixture.jobRepository_.findById(original.application_.id_);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->jobTitle_, original.application_.jobTitle_);
    QCOMPARE(stored->cvId_, original.application_.cvId_);
    QCOMPARE(fixture.cvRepository_.findAll().size(), 1);
    QCOMPARE(
        QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).size(),
        1);
}

void UpdateJobServiceTest::rejectsMissingAndInvalidApplicationsWithoutMutation()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto original = createInitialJob(fixture);
    QVERIFY(original.success_);

    const auto missing = updateJob(
        fixture,
        QStringLiteral("missing-job"),
        testsupport::validJobDraft());
    QVERIFY(!missing.success_);
    QVERIFY(missing.message_.contains(QStringLiteral("no longer exists")));

    auto invalidDraft = testsupport::validJobDraft();
    invalidDraft.jobTitle_.clear();
    invalidDraft.companyName_.clear();
    invalidDraft.status_ = QStringLiteral("Pending");
    invalidDraft.appliedDate_ = QStringLiteral("not-a-date");
    const auto preflight = JobApplicationValidator::preflight(invalidDraft, true);
    QVERIFY(!preflight.isValid());
    QVERIFY(preflight.fieldErrors_.contains(QStringLiteral("jobTitle")));
    QVERIFY(preflight.fieldErrors_.contains(QStringLiteral("companyName")));
    QVERIFY(preflight.fieldErrors_.contains(QStringLiteral("status")));
    QVERIFY(preflight.fieldErrors_.contains(QStringLiteral("appliedDate")));

    const auto cancellation = std::make_shared<CancellationState>();
    const auto preparation = fixture.updateService_.prepare(
        original.application_.id_,
        preflight.draft_,
        QUrl::fromLocalFile(fixture.storage_.createFile(
            QStringLiteral("invalid-replacement.pdf"))),
        cancellation);
    QVERIFY(!preparation.success_);
    QVERIFY(preparation.fieldErrors_.contains(QStringLiteral("jobTitle")));
    QCOMPARE(fixture.jobRepository_.findAll().size(), 1);
    QCOMPARE(fixture.cvRepository_.findAll().size(), 1);
    QCOMPARE(
        QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).size(),
        1);
}

void UpdateJobServiceTest::cancelsPreparedReplacementBeforeTransaction()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto original = createInitialJob(fixture);
    QVERIFY(original.success_);
    const auto replacementUrl = QUrl::fromLocalFile(fixture.storage_.createFile(
        QStringLiteral("cancelled-replacement.pdf"),
        QByteArrayLiteral("%PDF cancelled replacement")));
    const auto preflight = JobApplicationValidator::preflight(
        testsupport::validJobDraft(),
        true);
    QVERIFY(preflight.isValid());
    const auto cancellation = std::make_shared<CancellationState>();
    auto preparation = fixture.updateService_.prepare(
        original.application_.id_,
        preflight.draft_,
        replacementUrl,
        cancellation);
    QVERIFY2(preparation.success_, qPrintable(preparation.message_));
    cancellation->requestCancellation();

    const auto result = fixture.updateService_.complete(
        std::move(preparation),
        cancellation);

    QVERIFY(!result.success_);
    QVERIFY(result.message_.contains(QStringLiteral("canceled"), Qt::CaseInsensitive));
    const auto stored = fixture.jobRepository_.findById(original.application_.id_);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->cvId_, original.application_.cvId_);
    QCOMPARE(fixture.cvRepository_.findAll().size(), 1);
    QCOMPARE(
        QDir{fixture.storage_.paths().resumesDirectory()}.entryList(QDir::Files).size(),
        1);
}

void UpdateJobServiceTest::persistsUpdateAcrossDatabaseReopen()
{
    testsupport::AddJobTestFixture fixture;
    QVERIFY(fixture.isValid());
    const auto original = createInitialJob(fixture);
    QVERIFY(original.success_);
    auto draft = testsupport::validJobDraft();
    draft.jobTitle_ = QStringLiteral("Persisted After Restart");
    draft.status_ = QStringLiteral("Offer");
    draft.techStack_ = {
        QStringLiteral("C++"),
        QStringLiteral("Qt"),
        QStringLiteral("SQLite")};
    const auto result = updateJob(fixture, original.application_.id_, draft);
    QVERIFY2(result.success_, qPrintable(result.message_));

    SqliteDatabase reopenedDatabase{fixture.storage_.paths().databasePath()};
    JobRepository reopenedRepository{reopenedDatabase.connection()};
    const auto reloaded = reopenedRepository.findById(original.application_.id_);

    QVERIFY(reloaded.has_value());
    QCOMPARE(reloaded->jobTitle_, QStringLiteral("Persisted After Restart"));
    QCOMPARE(reloaded->status_, JobStatus::Offer);
    QCOMPARE(reloaded->techStack_, draft.techStack_);
    QCOMPARE(reloaded->id_, original.application_.id_);
    QCOMPARE(reloaded->createdAt_, original.application_.createdAt_);
}

QTEST_GUILESS_MAIN(UpdateJobServiceTest)

#include "UpdateJobServiceTest.moc"

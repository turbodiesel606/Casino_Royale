#include "common/LimitedSortedProxyModel.hpp"
#include "common/ExceptionUtils.hpp"
#include "common/RoleFilterProxyModel.hpp"
#include "jobs/JobApplicationFactory.hpp"
#include "jobs/JobApplicationValidator.hpp"

#include "../support/MutableRecordModel.hpp"

#include <QDateTime>
#include <QtTest/QtTest>

#include <exception>
#include <stdexcept>

class FilterAndValidationTest final : public QObject
{
    Q_OBJECT

private slots:
    void exceptionMessagesPreserveDetailsAndFallbacks();
    void proxyFiltersSearchTextAcrossConfiguredRoles();
    void proxyCombinesExactAndRequiredRoleFilters();
    void proxySortsStringAndNumericRoles();
    void limitedSortedProxyKeepsNewestRowsAcrossMutations();
    void jobEnumMappingsPreserveExistingTokens();
    void jobDraftNormalizationAppliesCanonicalDefaults();
    void jobPreflightNormalizesAndValidates();
    void jobValidationReturnsStructuredCanonicalErrors();
    void jobValidationAcceptsBlankOptionalUrl();
    void jobValidationRejectsMalformedNonEmptyUrls();
};

void FilterAndValidationTest::exceptionMessagesPreserveDetailsAndFallbacks()
{
    const auto fallback = QStringLiteral("Fallback message.");
    QCOMPARE(common::exceptionMessage({}, fallback), fallback);

    std::exception_ptr standardException;
    try {
        throw std::runtime_error("Detailed failure.");
    }
    catch (...) {
        standardException = std::current_exception();
    }
    QCOMPARE(
        common::exceptionMessage(standardException, fallback),
        QStringLiteral("Detailed failure."));

    std::exception_ptr unknownException;
    try {
        throw 42;
    }
    catch (...) {
        unknownException = std::current_exception();
    }
    QCOMPARE(common::exceptionMessage(unknownException, fallback), fallback);
}

void FilterAndValidationTest::proxyFiltersSearchTextAcrossConfiguredRoles()
{
    testsupport::MutableRecordModel model;
    model.addRow(QStringLiteral("Qt Developer"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("Product Designer"), QStringLiteral("Design"), {}, 1);
    model.addRow(QStringLiteral("QML Engineer"), QStringLiteral("Engineering"), QStringLiteral("LinkedIn"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setSearchRoles({
        testsupport::MutableRecordModel::NameRole,
        testsupport::MutableRecordModel::CategoryRole,
    });
    proxy.setSearchText(QStringLiteral("qml engineering"));

    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::NameRole).toString(),
        QStringLiteral("QML Engineer"));
}

void FilterAndValidationTest::proxyCombinesExactAndRequiredRoleFilters()
{
    testsupport::MutableRecordModel model;
    model.addRow(QStringLiteral("Qt Developer"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("Product Designer"), QStringLiteral("Design"), {}, 1);
    model.addRow(QStringLiteral("QML Engineer"), QStringLiteral("Engineering"), QStringLiteral("LinkedIn"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setExactFilter(
        testsupport::MutableRecordModel::CategoryRole,
        QStringLiteral("Engineering"));
    proxy.setRequiredNonEmptyRole(testsupport::MutableRecordModel::ChannelRole);
    QCOMPARE(proxy.rowCount(), 2);

    proxy.setExactFilter(
        testsupport::MutableRecordModel::CategoryRole,
        QStringLiteral("Design"));
    QCOMPARE(proxy.rowCount(), 0);
}

void FilterAndValidationTest::proxySortsStringAndNumericRoles()
{
    testsupport::MutableRecordModel model;
    model.addRow(QStringLiteral("Qt Developer"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("Product Designer"), QStringLiteral("Design"), {}, 1);
    model.addRow(QStringLiteral("QML Engineer"), QStringLiteral("Engineering"), QStringLiteral("LinkedIn"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    proxy.setSort(testsupport::MutableRecordModel::NameRole);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::NameRole).toString(),
        QStringLiteral("Product Designer"));

    proxy.setSort(testsupport::MutableRecordModel::ScoreRole, Qt::DescendingOrder);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::ScoreRole).toInt(),
        3);

    proxy.setSort(testsupport::MutableRecordModel::TimestampRole, Qt::DescendingOrder);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::ScoreRole).toInt(),
        3);
}

void FilterAndValidationTest::limitedSortedProxyKeepsNewestRowsAcrossMutations()
{
    testsupport::MutableRecordModel model;
    model.addRow(QStringLiteral("three"), QStringLiteral("Three"), QStringLiteral("Engineering"), QStringLiteral("Email"), 3);
    model.addRow(QStringLiteral("one"), QStringLiteral("One"), QStringLiteral("Engineering"), QStringLiteral("Email"), 1);
    model.addRow(QStringLiteral("seven"), QStringLiteral("Seven"), QStringLiteral("Engineering"), QStringLiteral("Email"), 7);
    model.addRow(QStringLiteral("two"), QStringLiteral("Two"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("six"), QStringLiteral("Six"), QStringLiteral("Engineering"), QStringLiteral("Email"), 6);
    model.addRow(QStringLiteral("four"), QStringLiteral("Four"), QStringLiteral("Engineering"), QStringLiteral("Email"), 4);
    model.addRow(QStringLiteral("five"), QStringLiteral("Five"), QStringLiteral("Engineering"), QStringLiteral("Email"), 5);

    LimitedSortedProxyModel proxy{
        model,
        testsupport::MutableRecordModel::TimestampRole,
        5,
        testsupport::MutableRecordModel::IdRole};
    proxy.setRoleName(
        testsupport::MutableRecordModel::TimestampRole,
        QByteArrayLiteral("recentTimestamp"));

    QCOMPARE(proxy.rowCount(), 5);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("seven"));
    QCOMPARE(
        proxy.data(proxy.index(4, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("three"));
    QCOMPARE(
        proxy.roleNames().value(testsupport::MutableRecordModel::TimestampRole),
        QByteArray{"recentTimestamp"});

    model.insertRowData(0, QStringLiteral("eight"), QStringLiteral("Eight"), 7);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("eight"));

    model.updateTimestamp(
        2,
        QDateTime{QDate{2026, 1, 10}, QTime{0, 0}, Qt::UTC});
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("one"));

    model.moveRowData(2, 7);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("one"));

    model.removeRowData(7);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("eight"));
}

void FilterAndValidationTest::jobEnumMappingsPreserveExistingTokens()
{
    const QList<QPair<JobStatus, QString>> statuses{
        {JobStatus::Applied, QStringLiteral("Applied")},
        {JobStatus::Interview, QStringLiteral("Interview")},
        {JobStatus::Offer, QStringLiteral("Offer")},
        {JobStatus::TestTask, QStringLiteral("Test Task")},
        {JobStatus::Rejected, QStringLiteral("Rejected")},
    };
    for (const auto& [status, text] : statuses) {
        QCOMPARE(jobStatusFromString(QStringLiteral("  ") + text.toLower()), status);
        QCOMPARE(jobStatusToString(status), text);
    }
    QCOMPARE(jobStatusFromString(QStringLiteral("Pending")), JobStatus::Unknown);
    QVERIFY(jobStatusToString(JobStatus::Unknown).isEmpty());

    const QList<QPair<WorkFormat, QString>> workFormats{
        {WorkFormat::Remote, QStringLiteral("Remote")},
        {WorkFormat::Hybrid, QStringLiteral("Hybrid")},
        {WorkFormat::OnSite, QStringLiteral("On-site")},
    };
    for (const auto& [workFormat, text] : workFormats) {
        QCOMPARE(workFormatFromString(QStringLiteral("  ") + text.toUpper()), workFormat);
        QCOMPARE(workFormatToString(workFormat), text);
    }
    QCOMPARE(workFormatFromString(QString{}), WorkFormat::Unspecified);
    QCOMPARE(workFormatFromString(QStringLiteral("Office")), WorkFormat::Unknown);
    QVERIFY(workFormatToString(WorkFormat::Unspecified).isEmpty());
    QVERIFY(workFormatToString(WorkFormat::Unknown).isEmpty());
}

void FilterAndValidationTest::jobDraftNormalizationAppliesCanonicalDefaults()
{
    JobApplicationDraft draft;
    draft.jobTitle_ = QStringLiteral("  Qt Developer  ");
    draft.companyName_ = QStringLiteral("  Example Company  ");
    draft.jobUrl_ = QStringLiteral("  https://example.com/jobs/qt  ");
    draft.workFormat_ = QStringLiteral(" remote ");
    draft.city_ = QStringLiteral("  Prague  ");
    draft.techStack_ = {
        QStringLiteral(" Qt "),
        QStringLiteral("qt"),
        QStringLiteral(" C++ "),
        QStringLiteral(" "),
    };

    const auto normalized = JobApplicationFactory::normalize(draft);

    QCOMPARE(normalized.jobTitle_, QStringLiteral("Qt Developer"));
    QCOMPARE(normalized.companyName_, QStringLiteral("Example Company"));
    QCOMPARE(normalized.jobUrl_, QUrl{QStringLiteral("https://example.com/jobs/qt")});
    QCOMPARE(normalized.workFormat_, WorkFormat::Remote);
    QCOMPARE(normalized.status_, JobStatus::Applied);
    QCOMPARE(normalized.appliedDate_, QDate::currentDate());
    QCOMPARE(normalized.city_, QStringLiteral("Prague"));
    QCOMPARE(
        normalized.techStack_,
        QStringList({QStringLiteral("Qt"), QStringLiteral("C++")}));
}

void FilterAndValidationTest::jobPreflightNormalizesAndValidates()
{
    JobApplicationDraft draft;
    draft.jobTitle_ = QStringLiteral("  Qt Developer  ");
    draft.companyName_ = QStringLiteral("  Example Company  ");
    draft.workFormat_ = QStringLiteral(" remote ");
    draft.status_ = QStringLiteral(" applied ");
    draft.appliedDate_ = QStringLiteral(" 2026-08-24 ");

    const auto valid = JobApplicationValidator::preflight(draft, true);
    QVERIFY2(valid.isValid(), qPrintable(valid.message_));
    QCOMPARE(valid.draft_.jobTitle_, QStringLiteral("Qt Developer"));
    QCOMPARE(valid.draft_.companyName_, QStringLiteral("Example Company"));
    QCOMPARE(valid.draft_.workFormat_, WorkFormat::Remote);
    QCOMPARE(valid.draft_.status_, JobStatus::Applied);
    QCOMPARE(valid.draft_.appliedDate_, QDate(2026, 8, 24));
    QVERIFY(valid.message_.isEmpty());

    const auto missingCv = JobApplicationValidator::preflight(draft, false);
    QVERIFY(!missingCv.isValid());
    QVERIFY(missingCv.fieldErrors_.contains(QStringLiteral("cv")));
    QCOMPARE(
        missingCv.message_,
        QStringLiteral("Please correct the highlighted fields."));
}

void FilterAndValidationTest::jobValidationReturnsStructuredCanonicalErrors()
{
    JobApplicationDraft validDraft;
    validDraft.jobTitle_ = QStringLiteral("Qt Developer");
    validDraft.companyName_ = QStringLiteral("Example Company");
    validDraft.status_ = QStringLiteral("Applied");
    validDraft.workFormat_ = QStringLiteral("Hybrid");
    validDraft.appliedDate_ = QStringLiteral("2026-07-09");
    const auto selectedCv = QUrl::fromLocalFile(QStringLiteral("C:/resume.pdf"));

    auto normalized = JobApplicationFactory::normalize(validDraft);
    QVERIFY(JobApplicationValidator::validate(normalized, selectedCv).isValid());

    auto invalidDraft = validDraft;
    invalidDraft.jobTitle_ = QStringLiteral(" ");
    invalidDraft.companyName_.clear();
    invalidDraft.jobUrl_ = QStringLiteral("https:job-posting");
    invalidDraft.workFormat_ = QStringLiteral("Office");
    invalidDraft.status_ = QStringLiteral("Pending");
    invalidDraft.appliedDate_ = QStringLiteral("2026-99-87");
    normalized = JobApplicationFactory::normalize(invalidDraft);
    const auto errors = JobApplicationValidator::validate(normalized, {});

    QVERIFY(!errors.isValid());
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
        QVERIFY2(errors.fieldErrors_.contains(field), qPrintable(field));
    }

    invalidDraft = validDraft;
    invalidDraft.jobUrl_ = QStringLiteral("ftp://example.com/job");
    const auto unsupportedScheme = JobApplicationValidator::validate(
        JobApplicationFactory::normalize(invalidDraft),
        selectedCv);
    QVERIFY(unsupportedScheme.fieldErrors_.contains(QStringLiteral("jobUrl")));

    JobApplication application;
    application.jobTitle_ = normalized.jobTitle_;
    application.companyName_ = normalized.companyName_;
    application.jobUrl_ = normalized.jobUrl_;
    application.workFormat_ = normalized.workFormat_;
    application.status_ = normalized.status_;
    application.appliedDate_ = normalized.appliedDate_;
    const auto selectedErrors = JobApplicationValidator::validate(application);
    QCOMPARE(selectedErrors.fieldErrors_.keys(), errors.fieldErrors_.keys());
}

void FilterAndValidationTest::jobValidationAcceptsBlankOptionalUrl()
{
    JobApplicationDraft draft;
    draft.jobTitle_ = QStringLiteral("Qt Developer");
    draft.companyName_ = QStringLiteral("Example Company");
    draft.workFormat_ = QStringLiteral("Remote");
    draft.status_ = QStringLiteral("Applied");
    draft.appliedDate_ = QStringLiteral("2026-08-04");

    const auto validation = JobApplicationValidator::validate(
        JobApplicationFactory::normalize(draft),
        QUrl::fromLocalFile(QStringLiteral("C:/resume.pdf")));

    QVERIFY(validation.isValid());
    QVERIFY(!validation.fieldErrors_.contains(QStringLiteral("jobUrl")));
}

void FilterAndValidationTest::jobValidationRejectsMalformedNonEmptyUrls()
{
    JobApplicationDraft draft;
    draft.jobTitle_ = QStringLiteral("Qt Developer");
    draft.companyName_ = QStringLiteral("Example Company");
    draft.workFormat_ = QStringLiteral("Remote");
    draft.status_ = QStringLiteral("Applied");
    draft.appliedDate_ = QStringLiteral("2026-08-04");
    const auto selectedCv = QUrl::fromLocalFile(QStringLiteral("C:/resume.pdf"));

    const QStringList invalidUrls{
        QStringLiteral("https:job-posting"),
        QStringLiteral("ftp://example.com/job"),
        QStringLiteral("file:///C:/job.txt"),
    };
    for (const auto& invalidUrl : invalidUrls) {
        draft.jobUrl_ = invalidUrl;
        const auto validation = JobApplicationValidator::validate(
            JobApplicationFactory::normalize(draft),
            selectedCv);
        QVERIFY2(
            validation.fieldErrors_.contains(QStringLiteral("jobUrl")),
            qPrintable(invalidUrl));
    }
}

QTEST_GUILESS_MAIN(FilterAndValidationTest)

#include "FilterAndValidationTest.moc"

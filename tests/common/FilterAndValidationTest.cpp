#include "common/LimitedSortedProxyModel.hpp"
#include "common/RoleFilterProxyModel.hpp"
#include "jobs/JobApplicationFactory.hpp"
#include "jobs/JobApplicationValidator.hpp"

#include "../support/MutableRecordModel.hpp"

#include <QDateTime>
#include <QtTest/QtTest>

class FilterAndValidationTest final : public QObject
{
    Q_OBJECT

private slots:
    void proxyFiltersSearchTextAcrossConfiguredRoles();
    void proxyCombinesExactAndRequiredRoleFilters();
    void proxySortsStringAndNumericRoles();
    void limitedSortedProxyKeepsNewestRowsAcrossMutations();
    void jobDraftNormalizationAppliesCanonicalDefaults();
    void jobValidationReturnsStructuredCanonicalErrors();
};

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

QTEST_GUILESS_MAIN(FilterAndValidationTest)

#include "FilterAndValidationTest.moc"

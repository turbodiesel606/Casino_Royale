#include "common/RelationFilterProxyModel.hpp"

#include "../support/MutableRecordModel.hpp"

#include <QtTest/QtTest>

class RelationFilterProxyModelTest final : public QObject
{
    Q_OBJECT

private slots:
    void tracksSourceMutationsAndPreservesRoles();
};

void RelationFilterProxyModelTest::tracksSourceMutationsAndPreservesRoles()
{
    testsupport::MutableRecordModel model;
    model.addRow(QStringLiteral("a"), QStringLiteral("Alpha"), QStringLiteral("Engineering"), QStringLiteral("Email"), 1);
    model.addRow(QStringLiteral("b"), QStringLiteral("Beta"), QStringLiteral("Design"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("c"), QStringLiteral("Gamma"), QStringLiteral("Engineering"), QStringLiteral("Email"), 3);

    RelationFilterProxyModel proxy{model, testsupport::MutableRecordModel::CategoryRole};
    QCOMPARE(proxy.rowCount(), 0);

    proxy.setSelectedId(QStringLiteral("Engineering"));
    QCOMPARE(proxy.rowCount(), 2);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("a"));
    QCOMPARE(
        proxy.data(proxy.index(1, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("c"));
    QCOMPARE(
        proxy.roleNames().value(testsupport::MutableRecordModel::NameRole),
        QByteArray{"name"});

    model.insertRowData(0, QStringLiteral("x"), QStringLiteral("Inserted"), 4);
    QCOMPARE(proxy.rowCount(), 3);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("x"));

    model.removeRowData(1);
    QCOMPARE(proxy.rowCount(), 2);
    QCOMPARE(
        proxy.data(proxy.index(1, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("c"));

    model.moveRowData(2, 0);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("c"));

    model.updateCategory(2, QStringLiteral("Engineering"));
    QCOMPARE(proxy.rowCount(), 3);
    QCOMPARE(
        proxy.data(proxy.index(2, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("b"));

    model.updateCategory(1, QStringLiteral("Design"));
    QCOMPARE(proxy.rowCount(), 2);

    model.resetRows({QStringLiteral("reset-a"), QStringLiteral("reset-b")});
    QCOMPARE(proxy.rowCount(), 2);
    QCOMPARE(
        proxy.data(proxy.index(0, 0), testsupport::MutableRecordModel::IdRole).toString(),
        QStringLiteral("reset-a"));

    proxy.setSelectedId(QStringLiteral("Missing"));
    QCOMPARE(proxy.rowCount(), 0);
}

QTEST_GUILESS_MAIN(RelationFilterProxyModelTest)

#include "RelationFilterProxyModelTest.moc"

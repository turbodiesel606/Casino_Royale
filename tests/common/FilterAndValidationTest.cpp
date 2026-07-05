#include "common/RoleFilterProxyModel.h"
#include "common/ValidationService.h"

#include <QAbstractListModel>
#include <QtTest/QtTest>

namespace {

class SimpleListModel final : public QAbstractListModel
{
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        CategoryRole,
        ChannelRole,
        ScoreRole,
    };

    explicit SimpleListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : rows_.size();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size()) {
            return {};
        }

        return rows_.at(index.row()).value(role);
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {NameRole, "name"},
            {CategoryRole, "category"},
            {ChannelRole, "channel"},
            {ScoreRole, "score"},
        };
    }

    void addRow(const QString& name, const QString& category, const QString& channel, int score)
    {
        const auto row = rows_.size();
        beginInsertRows(QModelIndex(), row, row);
        rows_.append({
            {NameRole, name},
            {CategoryRole, category},
            {ChannelRole, channel},
            {ScoreRole, score},
        });
        endInsertRows();
    }

private:
    QVector<QHash<int, QVariant>> rows_;
};

} // namespace

class FilterAndValidationTest final : public QObject
{
    Q_OBJECT

private slots:
    void proxyFiltersSearchTextAcrossConfiguredRoles();
    void proxyCombinesExactAndRequiredRoleFilters();
    void proxySortsStringAndNumericRoles();
    void validationReportsRequiredAndUrlErrors();
};

void FilterAndValidationTest::proxyFiltersSearchTextAcrossConfiguredRoles()
{
    SimpleListModel model;
    model.addRow(QStringLiteral("Qt Developer"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("Product Designer"), QStringLiteral("Design"), QString(), 1);
    model.addRow(QStringLiteral("QML Engineer"), QStringLiteral("Engineering"), QStringLiteral("LinkedIn"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setSearchRoles({SimpleListModel::NameRole, SimpleListModel::CategoryRole});

    proxy.setSearchText(QStringLiteral("qml engineering"));

    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.data(proxy.index(0, 0), SimpleListModel::NameRole).toString(), QStringLiteral("QML Engineer"));
}

void FilterAndValidationTest::proxyCombinesExactAndRequiredRoleFilters()
{
    SimpleListModel model;
    model.addRow(QStringLiteral("Qt Developer"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("Product Designer"), QStringLiteral("Design"), QString(), 1);
    model.addRow(QStringLiteral("QML Engineer"), QStringLiteral("Engineering"), QStringLiteral("LinkedIn"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setExactFilter(SimpleListModel::CategoryRole, QStringLiteral("Engineering"));
    proxy.setRequiredNonEmptyRole(SimpleListModel::ChannelRole);

    QCOMPARE(proxy.rowCount(), 2);

    proxy.setExactFilter(SimpleListModel::CategoryRole, QStringLiteral("Design"));
    QCOMPARE(proxy.rowCount(), 0);
}

void FilterAndValidationTest::proxySortsStringAndNumericRoles()
{
    SimpleListModel model;
    model.addRow(QStringLiteral("Qt Developer"), QStringLiteral("Engineering"), QStringLiteral("Email"), 2);
    model.addRow(QStringLiteral("Product Designer"), QStringLiteral("Design"), QString(), 1);
    model.addRow(QStringLiteral("QML Engineer"), QStringLiteral("Engineering"), QStringLiteral("LinkedIn"), 3);

    RoleFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    proxy.setSort(SimpleListModel::NameRole);
    QCOMPARE(proxy.data(proxy.index(0, 0), SimpleListModel::NameRole).toString(), QStringLiteral("Product Designer"));

    proxy.setSort(SimpleListModel::ScoreRole, Qt::DescendingOrder);
    QCOMPARE(proxy.data(proxy.index(0, 0), SimpleListModel::ScoreRole).toInt(), 3);
}

void FilterAndValidationTest::validationReportsRequiredAndUrlErrors()
{
    const auto required = ValidationService::validateRequiredFields(
        {{QStringLiteral("title"), QStringLiteral("Qt Developer")}, {QStringLiteral("company"), QString()}},
        {QStringLiteral("title"), QStringLiteral("company")});
    QVERIFY(!required.isValid_);
    QCOMPARE(required.messages_, QStringList({QStringLiteral("company is required.")}));

    const auto validUrl = ValidationService::validateHttpUrl(QStringLiteral("jobUrl"), QStringLiteral("https://example.com/job"), true);
    QVERIFY(validUrl.isValid_);
    QVERIFY(validUrl.messages_.isEmpty());

    const auto invalidUrl = ValidationService::validateHttpUrl(QStringLiteral("jobUrl"), QStringLiteral("ftp://example.com/job"), true);
    QVERIFY(!invalidUrl.isValid_);
    QCOMPARE(invalidUrl.messages_, QStringList({QStringLiteral("jobUrl must be a valid HTTP or HTTPS URL.")}));
}

QTEST_APPLESS_MAIN(FilterAndValidationTest)

#include "FilterAndValidationTest.moc"

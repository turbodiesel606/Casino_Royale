#include "directory/CompanyDirectoryController.hpp"
#include "directory/ContactDirectoryController.hpp"
#include "directory/ContactListModel.hpp"
#include "jobs/JobApplicationListModel.hpp"

#include "../support/JobApplicationTestData.hpp"

#include <QSignalSpy>
#include <QtTest/QtTest>

namespace {

int roleForName(const QAbstractItemModel& model, const QByteArray& roleName)
{
    const auto names = model.roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        if (it.value() == roleName) {
            return it.key();
        }
    }
    return -1;
}

}

class DirectoryControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void companyModelExposesNamedRoles();
    void contactModelExposesNamedRoles();
};

void DirectoryControllerTest::companyModelExposesNamedRoles()
{
    JobApplicationListModel applicationsModel;
    ContactListModel contactModel;
    CompanyDirectoryController controller(applicationsModel, contactModel);
    const auto* model = controller.companyModel();

    QVERIFY(roleForName(*model, "id") > 0);
    QVERIFY(roleForName(*model, "name") > 0);
    QVERIFY(roleForName(*model, "website") > 0);
    QVERIFY(roleForName(*model, "logoText") > 0);
    QVERIFY(roleForName(*model, "openJobCountLabel") > 0);
    QVERIFY(roleForName(*model, "contactCountLabel") > 0);
    QVERIFY(roleForName(*model, "lastActivityLabel") > 0);

    QCOMPARE(model->rowCount(), 0);
    QVERIFY(controller.selectedCompanyId().isEmpty());
    QCOMPARE(controller.resultSummary(), QStringLiteral("Showing 0 companies"));
}

void DirectoryControllerTest::contactModelExposesNamedRoles()
{
    ContactListModel contactModel;
    ContactDirectoryController controller(contactModel);
    const auto* model = controller.contactModel();

    QVERIFY(roleForName(*model, "id") > 0);
    QVERIFY(roleForName(*model, "displayName") > 0);
    QVERIFY(roleForName(*model, "roleTitle") > 0);
    QVERIFY(roleForName(*model, "companyId") > 0);
    QVERIFY(roleForName(*model, "relatedApplicationTitle") > 0);
    QVERIFY(roleForName(*model, "email") > 0);
    QVERIFY(roleForName(*model, "linkedin") > 0);

    QCOMPARE(model->rowCount(), 0);
    QVERIFY(controller.selectedContactId().isEmpty());
    QCOMPARE(controller.resultSummary(), QStringLiteral("0 contacts"));
}

QTEST_APPLESS_MAIN(DirectoryControllerTest)

#include "DirectoryControllerTest.moc"

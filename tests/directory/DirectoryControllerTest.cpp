#include "directory/CompanyDirectoryController.h"
#include "directory/ContactDirectoryController.h"
#include "directory/ContactListModel.h"
#include "jobs/JobApplicationListModel.h"

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
    void companySelectionExposesLinkedJobsAndContacts();
    void contactModelExposesNamedRoles();
    void contactSelectionExposesInteractionHistory();
    void companyControllerFiltersAndSortsCompanies();
    void contactControllerFiltersAndSortsContacts();
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

    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-kdab"));
    QCOMPARE(controller.resultSummary(), QStringLiteral("Showing 1 to 5 of 5 companies"));
}

void DirectoryControllerTest::companySelectionExposesLinkedJobsAndContacts()
{
    JobApplicationListModel applicationsModel;
    ContactListModel contactModel;
    CompanyDirectoryController controller(applicationsModel, contactModel);
    QSignalSpy selectedSpy(&controller, &CompanyDirectoryController::selectedCompanyChanged);
    QSignalSpy linkedSpy(&controller, &CompanyDirectoryController::linkedModelsChanged);

    controller.selectCompany(1);

    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(linkedSpy.count(), 1);
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-techsoft"));
    QCOMPARE(controller.selectedCompany().value(QStringLiteral("name")).toString(), QStringLiteral("TechSoft"));

    const auto* jobsModel = controller.linkedJobsModel();
    QCOMPARE(jobsModel->rowCount(), 1);
    QCOMPARE(jobsModel->data(jobsModel->index(0, 0), roleForName(*jobsModel, "jobTitle")).toString(), QStringLiteral("Qt/QML Engineer"));

    const auto* contactsModel = controller.linkedContactsModel();
    QCOMPARE(contactsModel->rowCount(), 1);
    QCOMPARE(contactsModel->data(contactsModel->index(0, 0), roleForName(*contactsModel, "displayName")).toString(), QStringLiteral("Maria Novak"));
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

    QCOMPARE(model->rowCount(), 4);
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-anna-mueller"));
    QCOMPARE(controller.resultSummary(), QStringLiteral("4 contacts"));
}

void DirectoryControllerTest::contactSelectionExposesInteractionHistory()
{
    ContactListModel contactModel;
    ContactDirectoryController controller(contactModel);
    QSignalSpy selectedSpy(&controller, &ContactDirectoryController::selectedContactChanged);
    QSignalSpy historySpy(&controller, &ContactDirectoryController::interactionHistoryModelChanged);

    controller.selectContact(2);

    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(historySpy.count(), 1);
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-maria-techsoft"));
    QCOMPARE(controller.selectedContact().value(QStringLiteral("companyName")).toString(), QStringLiteral("TechSoft"));
    QCOMPARE(controller.selectedContact().value(QStringLiteral("relatedApplicationTitle")).toString(), QStringLiteral("Qt/QML Engineer"));

    const auto* historyModel = controller.interactionHistoryModel();
    QCOMPARE(historyModel->rowCount(), 1);
    QCOMPARE(historyModel->data(historyModel->index(0, 0), roleForName(*historyModel, "title")).toString(), QStringLiteral("Interview schedule"));
}

void DirectoryControllerTest::companyControllerFiltersAndSortsCompanies()
{
    JobApplicationListModel applicationsModel;
    ContactListModel contactModel;
    CompanyDirectoryController controller(applicationsModel, contactModel);

    controller.setSearchText(QStringLiteral("TechSoft"));

    QCOMPARE(controller.companyCount(), 1);
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-techsoft"));
    QCOMPARE(controller.resultSummary(), QStringLiteral("Showing 1 company"));

    controller.clearFilters();
    controller.setSortMode(QStringLiteral("Contacts"));

    QCOMPARE(controller.companyCount(), 5);
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-kdab"));
    QCOMPARE(controller.companyModel()->data(controller.companyModel()->index(0, 0), roleForName(*controller.companyModel(), "contactCountLabel")).toString(), QStringLiteral("2 contacts"));
}

void DirectoryControllerTest::contactControllerFiltersAndSortsContacts()
{
    ContactListModel contactModel;
    ContactDirectoryController controller(contactModel);

    controller.setCompanyFilter(QStringLiteral("KDAB"));

    QCOMPARE(controller.contactCount(), 2);
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-anna-mueller"));

    controller.clearFilters();
    controller.setChannelFilter(QStringLiteral("Telegram"));

    QCOMPARE(controller.contactCount(), 3);

    controller.clearFilters();
    controller.setSortMode(QStringLiteral("Company"));

    QCOMPARE(controller.contactCount(), 4);
    QCOMPARE(controller.selectedContact().value(QStringLiteral("companyName")).toString(), QStringLiteral("KDAB"));
}

QTEST_APPLESS_MAIN(DirectoryControllerTest)

#include "DirectoryControllerTest.moc"

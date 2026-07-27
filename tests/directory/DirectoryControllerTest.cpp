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

QVector<Company> makeCompanies()
{
    Company beta;
    beta.id_ = QStringLiteral("company-beta");
    beta.name_ = QStringLiteral("Beta");
    beta.contactCount_ = 2;

    Company alpha;
    alpha.id_ = QStringLiteral("company-alpha");
    alpha.name_ = QStringLiteral("Alpha");
    alpha.contactCount_ = 1;

    Company gamma;
    gamma.id_ = QStringLiteral("company-gamma");
    gamma.name_ = QStringLiteral("Gamma");
    gamma.contactCount_ = 3;

    return {beta, alpha, gamma};
}

QVector<Contact> makeContacts()
{
    Contact bob;
    bob.id_ = QStringLiteral("contact-bob");
    bob.displayName_ = QStringLiteral("Bob");
    bob.companyId_ = QStringLiteral("company-beta");
    bob.companyName_ = QStringLiteral("Beta");
    bob.telegram_ = QStringLiteral("@bob");
    bob.lastContactLabel_ = QStringLiteral("May 1, 2026");
    bob.interactions_ = {
        {QStringLiteral("Message"), QStringLiteral("Introduction"), QStringLiteral("May 1, 2026"), QString()},
        {QStringLiteral("Call"), QStringLiteral("Follow-up"), QStringLiteral("May 2, 2026"), QString()}};

    Contact alice;
    alice.id_ = QStringLiteral("contact-alice");
    alice.displayName_ = QStringLiteral("Alice");
    alice.companyId_ = QStringLiteral("company-alpha");
    alice.companyName_ = QStringLiteral("Acme");
    alice.email_ = QStringLiteral("alice@example.test");
    alice.lastContactLabel_ = QStringLiteral("May 2, 2026");
    alice.interactions_ = {
        {QStringLiteral("Email"), QStringLiteral("Application"), QStringLiteral("May 2, 2026"), QString()}};

    Contact cara;
    cara.id_ = QStringLiteral("contact-cara");
    cara.displayName_ = QStringLiteral("Cara");
    cara.companyId_ = QStringLiteral("company-alpha");
    cara.companyName_ = QStringLiteral("Acme");
    cara.linkedin_ = QStringLiteral("linkedin.example/cara");
    cara.lastContactLabel_ = QStringLiteral("May 3, 2026");

    return {bob, alice, cara};
}

}

class DirectoryControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void companyModelExposesNamedRoles();
    void contactModelExposesNamedRoles();
    void companySelectionRemainsStableAcrossProxyChanges();
    void contactSelectionRemainsStableAcrossProxyChanges();
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
    QCOMPARE(controller.selectedCompanyIndex(), -1);
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
    QCOMPARE(controller.selectedContactIndex(), -1);
    QVERIFY(controller.selectedContactId().isEmpty());
    QCOMPARE(controller.resultSummary(), QStringLiteral("0 contacts"));
}

void DirectoryControllerTest::companySelectionRemainsStableAcrossProxyChanges()
{
    JobApplicationListModel applicationsModel;
    ContactListModel contactModel;
    CompanyDirectoryController controller{
        makeCompanies(),
        applicationsModel,
        contactModel};
    controller.selectCompany(1);
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-alpha"));

    QSignalSpy selectedSpy{&controller, &CompanyDirectoryController::selectedCompanyChanged};
    QSignalSpy linkedSpy{&controller, &CompanyDirectoryController::linkedModelsChanged};

    controller.setSortMode(QStringLiteral("Contacts"));
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-alpha"));
    QCOMPARE(controller.selectedCompanyIndex(), 2);
    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(linkedSpy.count(), 0);

    controller.setSearchText(QStringLiteral("Alpha"));
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-alpha"));
    QCOMPARE(controller.selectedCompanyIndex(), 0);
    QCOMPARE(linkedSpy.count(), 0);

    controller.clearFilters();
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-alpha"));
    QCOMPARE(controller.selectedCompanyIndex(), 2);
    QCOMPARE(linkedSpy.count(), 0);

    controller.setSearchText(QStringLiteral("Beta"));
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-beta"));
    QCOMPARE(controller.selectedCompanyIndex(), 0);
    QCOMPARE(linkedSpy.count(), 1);

    controller.clearFilters();
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-beta"));
    QCOMPARE(controller.selectedCompanyIndex(), 1);
    QCOMPARE(linkedSpy.count(), 1);

    controller.setSearchText(QStringLiteral("does-not-match"));
    QCOMPARE(controller.companyCount(), 0);
    QCOMPARE(controller.selectedCompanyIndex(), -1);
    QVERIFY(controller.selectedCompanyId().isEmpty());
    QCOMPARE(linkedSpy.count(), 2);

    controller.clearFilters();
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-gamma"));
    QCOMPARE(controller.selectedCompanyIndex(), 0);
    QCOMPARE(linkedSpy.count(), 3);

    controller.setSortMode(QStringLiteral("Name"));
    controller.selectCompany(1);
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-beta"));
    selectedSpy.clear();
    linkedSpy.clear();

    controller.publishCompany(
        QStringLiteral("company-aardvark"),
        QStringLiteral("Aardvark"));

    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-beta"));
    QCOMPARE(controller.selectedCompanyIndex(), 2);
    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(linkedSpy.count(), 0);
}

void DirectoryControllerTest::contactSelectionRemainsStableAcrossProxyChanges()
{
    ContactListModel contactModel{makeContacts()};
    ContactDirectoryController controller{contactModel};
    controller.selectContact(1);
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-alice"));

    QSignalSpy selectedSpy{&controller, &ContactDirectoryController::selectedContactChanged};
    QSignalSpy interactionSpy{&controller, &ContactDirectoryController::interactionHistoryModelChanged};

    controller.setSortMode(QStringLiteral("Company"));
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-alice"));
    QCOMPARE(controller.selectedContactIndex(), 0);
    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(interactionSpy.count(), 0);

    controller.setCompanyFilter(QStringLiteral("Acme"));
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-alice"));
    QCOMPARE(controller.selectedContactIndex(), 0);
    QCOMPARE(interactionSpy.count(), 0);

    controller.clearFilters();
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-alice"));
    QCOMPARE(controller.selectedContactIndex(), 0);
    QCOMPARE(interactionSpy.count(), 0);

    controller.setChannelFilter(QStringLiteral("Telegram"));
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-bob"));
    QCOMPARE(controller.selectedContactIndex(), 0);
    QCOMPARE(controller.interactionHistoryModel()->rowCount(), 2);
    QCOMPARE(interactionSpy.count(), 1);

    controller.clearFilters();
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-bob"));
    QCOMPARE(controller.selectedContactIndex(), 2);
    QCOMPARE(interactionSpy.count(), 1);

    controller.setSearchText(QStringLiteral("does-not-match"));
    QCOMPARE(controller.contactCount(), 0);
    QCOMPARE(controller.selectedContactIndex(), -1);
    QVERIFY(controller.selectedContactId().isEmpty());
    QCOMPARE(controller.interactionHistoryModel()->rowCount(), 0);
    QCOMPARE(interactionSpy.count(), 2);

    controller.clearFilters();
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-alice"));
    QCOMPARE(controller.selectedContactIndex(), 0);
    QCOMPARE(controller.interactionHistoryModel()->rowCount(), 1);
    QCOMPARE(interactionSpy.count(), 3);
}

QTEST_APPLESS_MAIN(DirectoryControllerTest)

#include "DirectoryControllerTest.moc"

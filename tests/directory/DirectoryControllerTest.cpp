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
    void scalarNotificationsAreSemantic();
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
    QVERIFY(roleForName(*model, "createdAt") > 0);
    QVERIFY(roleForName(*model, "updatedAt") > 0);

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

void DirectoryControllerTest::scalarNotificationsAreSemantic()
{
    JobApplicationListModel applicationsModel;
    ContactListModel contactModel{makeContacts()};
    CompanyDirectoryController companyController{
        makeCompanies(),
        applicationsModel,
        contactModel};
    QSignalSpy companySearchSpy{&companyController, &CompanyDirectoryController::searchTextChanged};
    QSignalSpy companySortSpy{&companyController, &CompanyDirectoryController::sortModeChanged};
    QSignalSpy companyCountSpy{&companyController, &CompanyDirectoryController::companyCountChanged};
    QSignalSpy companySummarySpy{&companyController, &CompanyDirectoryController::resultSummaryChanged};

    companyController.setSearchText(QStringLiteral("Alpha"));
    QCOMPARE(companySearchSpy.count(), 1);
    QCOMPARE(companyCountSpy.count(), 1);
    QCOMPARE(companySummarySpy.count(), 1);

    companyController.setSearchText(QStringLiteral("Alpha"));
    companyController.setSortMode(QStringLiteral("Contacts"));
    QCOMPARE(companySearchSpy.count(), 1);
    QCOMPARE(companySortSpy.count(), 1);
    QCOMPARE(companyCountSpy.count(), 1);
    QCOMPARE(companySummarySpy.count(), 1);

    companyController.clearFilters();
    QCOMPARE(companySearchSpy.count(), 2);
    QCOMPARE(companyCountSpy.count(), 2);
    QCOMPARE(companySummarySpy.count(), 2);

    ContactDirectoryController contactController{contactModel};
    QSignalSpy companyFilterSpy{&contactController, &ContactDirectoryController::companyFilterChanged};
    QSignalSpy channelFilterSpy{&contactController, &ContactDirectoryController::channelFilterChanged};
    QSignalSpy contactSortSpy{&contactController, &ContactDirectoryController::sortModeChanged};
    QSignalSpy contactCountSpy{&contactController, &ContactDirectoryController::contactCountChanged};
    QSignalSpy contactSummarySpy{&contactController, &ContactDirectoryController::resultSummaryChanged};

    contactController.setCompanyFilter(QStringLiteral("Acme"));
    contactController.setChannelFilter(QStringLiteral("Email"));
    QCOMPARE(companyFilterSpy.count(), 1);
    QCOMPARE(channelFilterSpy.count(), 1);
    QCOMPARE(contactCountSpy.count(), 2);
    QCOMPARE(contactSummarySpy.count(), 2);

    contactController.setSortMode(QStringLiteral("Company"));
    QCOMPARE(contactSortSpy.count(), 1);
    QCOMPARE(contactCountSpy.count(), 2);
    QCOMPARE(contactSummarySpy.count(), 2);

    contactController.clearFilters();
    QCOMPARE(companyFilterSpy.count(), 2);
    QCOMPARE(channelFilterSpy.count(), 2);
    QCOMPARE(contactCountSpy.count(), 3);
    QCOMPARE(contactSummarySpy.count(), 3);
}

void DirectoryControllerTest::companySelectionRemainsStableAcrossProxyChanges()
{
    auto applications = testsupport::makeJobApplications();
    applications[0].companyId_ = QStringLiteral("company-beta");
    applications[1].companyId_ = QStringLiteral("company-alpha");
    applications[2].companyId_ = QStringLiteral("company-alpha");
    applications[3].companyId_ = QStringLiteral("company-gamma");
    applications[4].companyId_ = QStringLiteral("company-gamma");
    applications[5].companyId_ = QStringLiteral("company-gamma");
    JobApplicationListModel applicationsModel{applications};
    ContactListModel contactModel{makeContacts()};
    CompanyDirectoryController controller{
        makeCompanies(),
        applicationsModel,
        contactModel};
    controller.selectCompany(1);
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-alpha"));

    QSignalSpy selectedIndexSpy{&controller, &CompanyDirectoryController::selectedCompanyIndexChanged};
    QSignalSpy selectedIdSpy{&controller, &CompanyDirectoryController::selectedCompanyIdChanged};
    QSignalSpy selectedDataSpy{&controller, &CompanyDirectoryController::selectedCompanyChanged};
    const auto* linkedJobsModel = controller.linkedJobsModel();
    const auto* linkedContactsModel = controller.linkedContactsModel();
    QCOMPARE(linkedJobsModel->rowCount(), 2);
    QCOMPARE(linkedContactsModel->rowCount(), 2);

    controller.setSortMode(QStringLiteral("Contacts"));
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-alpha"));
    QCOMPARE(controller.selectedCompanyIndex(), 2);
    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedIdSpy.count(), 0);
    QCOMPARE(selectedDataSpy.count(), 0);
    QCOMPARE(linkedJobsModel->rowCount(), 2);
    QCOMPARE(linkedContactsModel->rowCount(), 2);

    controller.setSearchText(QStringLiteral("Alpha"));
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-alpha"));
    QCOMPARE(controller.selectedCompanyIndex(), 0);
    QCOMPARE(linkedJobsModel->rowCount(), 2);
    QCOMPARE(linkedContactsModel->rowCount(), 2);

    controller.clearFilters();
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-alpha"));
    QCOMPARE(controller.selectedCompanyIndex(), 2);
    QCOMPARE(linkedJobsModel->rowCount(), 2);
    QCOMPARE(linkedContactsModel->rowCount(), 2);

    controller.setSearchText(QStringLiteral("Beta"));
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-beta"));
    QCOMPARE(controller.selectedCompanyIndex(), 0);
    QCOMPARE(linkedJobsModel->rowCount(), 1);
    QCOMPARE(linkedContactsModel->rowCount(), 1);

    controller.clearFilters();
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-beta"));
    QCOMPARE(controller.selectedCompanyIndex(), 1);
    QCOMPARE(linkedJobsModel->rowCount(), 1);
    QCOMPARE(linkedContactsModel->rowCount(), 1);

    controller.setSearchText(QStringLiteral("does-not-match"));
    QCOMPARE(controller.companyCount(), 0);
    QCOMPARE(controller.selectedCompanyIndex(), -1);
    QVERIFY(controller.selectedCompanyId().isEmpty());
    QCOMPARE(linkedJobsModel->rowCount(), 0);
    QCOMPARE(linkedContactsModel->rowCount(), 0);

    controller.clearFilters();
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-gamma"));
    QCOMPARE(controller.selectedCompanyIndex(), 0);
    QCOMPARE(linkedJobsModel->rowCount(), 3);
    QCOMPARE(linkedContactsModel->rowCount(), 0);

    controller.setSortMode(QStringLiteral("Name"));
    controller.selectCompany(1);
    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-beta"));
    selectedIndexSpy.clear();
    selectedIdSpy.clear();
    selectedDataSpy.clear();

    controller.publishCompany(
        QStringLiteral("company-aardvark"),
        QStringLiteral("Aardvark"));

    QCOMPARE(controller.selectedCompanyId(), QStringLiteral("company-beta"));
    QCOMPARE(controller.selectedCompanyIndex(), 2);
    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedIdSpy.count(), 0);
    QCOMPARE(selectedDataSpy.count(), 0);
    QCOMPARE(linkedJobsModel->rowCount(), 1);
    QCOMPARE(linkedContactsModel->rowCount(), 1);
}

void DirectoryControllerTest::contactSelectionRemainsStableAcrossProxyChanges()
{
    ContactListModel contactModel{makeContacts()};
    ContactDirectoryController controller{contactModel};
    controller.selectContact(1);
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-alice"));

    QSignalSpy selectedIndexSpy{&controller, &ContactDirectoryController::selectedContactIndexChanged};
    QSignalSpy selectedIdSpy{&controller, &ContactDirectoryController::selectedContactIdChanged};
    QSignalSpy selectedDataSpy{&controller, &ContactDirectoryController::selectedContactChanged};
    QSignalSpy interactionResetSpy{controller.interactionHistoryModel(), &QAbstractItemModel::modelReset};

    controller.setSortMode(QStringLiteral("Company"));
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-alice"));
    QCOMPARE(controller.selectedContactIndex(), 0);
    QCOMPARE(selectedIndexSpy.count(), 1);
    QCOMPARE(selectedIdSpy.count(), 0);
    QCOMPARE(selectedDataSpy.count(), 0);
    QCOMPARE(interactionResetSpy.count(), 0);

    controller.setCompanyFilter(QStringLiteral("Acme"));
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-alice"));
    QCOMPARE(controller.selectedContactIndex(), 0);
    QCOMPARE(interactionResetSpy.count(), 0);

    controller.clearFilters();
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-alice"));
    QCOMPARE(controller.selectedContactIndex(), 0);
    QCOMPARE(interactionResetSpy.count(), 0);

    controller.setChannelFilter(QStringLiteral("Telegram"));
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-bob"));
    QCOMPARE(controller.selectedContactIndex(), 0);
    QCOMPARE(controller.interactionHistoryModel()->rowCount(), 2);
    QCOMPARE(interactionResetSpy.count(), 1);

    controller.clearFilters();
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-bob"));
    QCOMPARE(controller.selectedContactIndex(), 2);
    QCOMPARE(interactionResetSpy.count(), 1);

    controller.setSearchText(QStringLiteral("does-not-match"));
    QCOMPARE(controller.contactCount(), 0);
    QCOMPARE(controller.selectedContactIndex(), -1);
    QVERIFY(controller.selectedContactId().isEmpty());
    QCOMPARE(controller.interactionHistoryModel()->rowCount(), 0);
    QCOMPARE(interactionResetSpy.count(), 2);

    controller.clearFilters();
    QCOMPARE(controller.selectedContactId(), QStringLiteral("contact-alice"));
    QCOMPARE(controller.selectedContactIndex(), 0);
    QCOMPARE(controller.interactionHistoryModel()->rowCount(), 1);
    QCOMPARE(interactionResetSpy.count(), 3);
}

QTEST_APPLESS_MAIN(DirectoryControllerTest)

#include "DirectoryControllerTest.moc"

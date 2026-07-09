#include "cvs/CvLibraryController.h"
#include "jobs/JobApplicationListModel.h"

#include "../support/JobApplicationTestData.h"

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

class CvLibraryControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void cvModelExposesNamedRoles();
    void cvModelExposesSeedDocuments();
    void selectedCvControlsLinkedApplications();
    void favoriteToggleUpdatesSelectedCv();
    void controllerFiltersAndSortsCvs();
};

void CvLibraryControllerTest::cvModelExposesNamedRoles()
{
    JobApplicationListModel applicationsModel;
    CvLibraryController controller(applicationsModel);
    const auto* model = controller.cvModel();

    QVERIFY(roleForName(*model, "id") > 0);
    QVERIFY(roleForName(*model, "fileName") > 0);
    QVERIFY(roleForName(*model, "categoryAccent") > 0);
    QVERIFY(roleForName(*model, "languageAccent") > 0);
    QVERIFY(roleForName(*model, "linkedApplicationCount") > 0);
    QVERIFY(roleForName(*model, "linkedApplicationCountLabel") > 0);
    QVERIFY(roleForName(*model, "isFavorite") > 0);
}

void CvLibraryControllerTest::cvModelExposesSeedDocuments()
{
    JobApplicationListModel applicationsModel;
    CvLibraryController controller(applicationsModel);
    const auto* model = controller.cvModel();
    const auto firstRow = model->index(0, 0);

    QCOMPARE(model->rowCount(), 4);
    QCOMPARE(model->data(firstRow, roleForName(*model, "id")).toString(), QStringLiteral("cv-qt-2026"));
    QCOMPARE(model->data(firstRow, roleForName(*model, "fileName")).toString(), QStringLiteral("CV_Qt_2026.pdf"));
    QCOMPARE(model->data(firstRow, roleForName(*model, "linkedApplicationCount")).toInt(), 4);
    QCOMPARE(controller.resultSummary(), QStringLiteral("4 CVs"));
}

void CvLibraryControllerTest::selectedCvControlsLinkedApplications()
{
    JobApplicationListModel applicationsModel(testsupport::makeJobApplications());
    CvLibraryController controller(applicationsModel);
    QSignalSpy selectedSpy(&controller, &CvLibraryController::selectedCvChanged);
    QSignalSpy linkedSpy(&controller, &CvLibraryController::linkedApplicationsModelChanged);

    controller.selectCv(2);

    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(linkedSpy.count(), 1);
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-general"));
    QCOMPARE(controller.selectedCv().value(QStringLiteral("fileName")).toString(), QStringLiteral("CV_General.pdf"));

    const auto* linkedModel = controller.linkedApplicationsModel();
    QCOMPARE(linkedModel->rowCount(), 3);
    const auto firstLinkedRow = linkedModel->index(0, 0);
    QCOMPARE(linkedModel->data(firstLinkedRow, roleForName(*linkedModel, "id")).toString(), QStringLiteral("job-greenwidget-software"));
    QCOMPARE(linkedModel->data(firstLinkedRow, roleForName(*linkedModel, "cvFileName")).toString(), QStringLiteral("CV_General.pdf"));
}

void CvLibraryControllerTest::favoriteToggleUpdatesSelectedCv()
{
    JobApplicationListModel applicationsModel;
    CvLibraryController controller(applicationsModel);
    QSignalSpy selectedSpy(&controller, &CvLibraryController::selectedCvChanged);

    QVERIFY(controller.selectedCv().value(QStringLiteral("isFavorite")).toBool());
    controller.toggleFavorite(QStringLiteral("cv-qt-2026"));

    QCOMPARE(selectedSpy.count(), 1);
    QVERIFY(!controller.selectedCv().value(QStringLiteral("isFavorite")).toBool());
}

void CvLibraryControllerTest::controllerFiltersAndSortsCvs()
{
    JobApplicationListModel applicationsModel(testsupport::makeJobApplications());
    CvLibraryController controller(applicationsModel);

    controller.setSearchText(QStringLiteral("embedded"));

    QCOMPARE(controller.cvCount(), 1);
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-embedded"));
    QCOMPARE(controller.resultSummary(), QStringLiteral("1 CV"));

    controller.clearFilters();
    controller.setCategoryFilter(QStringLiteral("General"));

    QCOMPARE(controller.cvCount(), 1);
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-general"));

    controller.clearFilters();
    controller.setSortMode(QStringLiteral("Linked Jobs"));

    QCOMPARE(controller.cvCount(), 4);
    QCOMPARE(controller.selectedCvId(), QStringLiteral("cv-qt-2026"));
    QCOMPARE(controller.cvModel()->data(controller.cvModel()->index(0, 0), roleForName(*controller.cvModel(), "linkedApplicationCount")).toInt(), 4);
}

QTEST_APPLESS_MAIN(CvLibraryControllerTest)

#include "CvLibraryControllerTest.moc"

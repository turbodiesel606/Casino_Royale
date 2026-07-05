#include "dashboard/DashboardController.h"

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

} // namespace

class DashboardControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void statsModelAggregatesJobStatuses();
    void funnelModelExposesRatios();
    void recentModelsExposeBackendRows();
};

void DashboardControllerTest::statsModelAggregatesJobStatuses()
{
    JobApplicationListModel applicationsModel;
    CvListModel cvModel;
    DashboardController controller(applicationsModel, cvModel);
    const auto* model = controller.statsModel();

    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->data(model->index(0, 0), roleForName(*model, "title")).toString(), QStringLiteral("Total Jobs"));
    QCOMPARE(model->data(model->index(0, 0), roleForName(*model, "value")).toString(), QStringLiteral("10"));
    QCOMPARE(model->data(model->index(1, 0), roleForName(*model, "title")).toString(), QStringLiteral("Applied"));
    QCOMPARE(model->data(model->index(1, 0), roleForName(*model, "value")).toString(), QStringLiteral("3"));
    QCOMPARE(model->data(model->index(2, 0), roleForName(*model, "value")).toString(), QStringLiteral("3"));
    QCOMPARE(model->data(model->index(3, 0), roleForName(*model, "value")).toString(), QStringLiteral("1"));
    QCOMPARE(model->data(model->index(4, 0), roleForName(*model, "value")).toString(), QStringLiteral("8"));
}

void DashboardControllerTest::funnelModelExposesRatios()
{
    JobApplicationListModel applicationsModel;
    CvListModel cvModel;
    DashboardController controller(applicationsModel, cvModel);
    const auto* model = controller.funnelModel();

    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->data(model->index(0, 0), roleForName(*model, "ratio")).toDouble(), 1.0);
    QCOMPARE(model->data(model->index(1, 0), roleForName(*model, "value")).toString(), QStringLiteral("3 (30.0%)"));
}

void DashboardControllerTest::recentModelsExposeBackendRows()
{
    JobApplicationListModel applicationsModel;
    CvListModel cvModel;
    DashboardController controller(applicationsModel, cvModel);

    const auto* applications = controller.recentApplicationsModel();
    QCOMPARE(applications->rowCount(), 5);
    QCOMPARE(applications->data(applications->index(0, 0), roleForName(*applications, "jobTitle")).toString(), QStringLiteral("C++/Qt Developer"));
    QCOMPARE(applications->data(applications->index(0, 0), roleForName(*applications, "companyName")).toString(), QStringLiteral("KDAB"));

    const auto* cvs = controller.recentCvsModel();
    QCOMPARE(cvs->rowCount(), 4);
    QCOMPARE(cvs->data(cvs->index(0, 0), roleForName(*cvs, "fileName")).toString(), QStringLiteral("CV_Qt_2026.pdf"));
    QCOMPARE(cvs->data(cvs->index(0, 0), roleForName(*cvs, "linkedApplicationCountLabel")).toString(), QStringLiteral("4 jobs"));
}

QTEST_APPLESS_MAIN(DashboardControllerTest)

#include "DashboardControllerTest.moc"

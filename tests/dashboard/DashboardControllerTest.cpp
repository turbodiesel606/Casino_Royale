#include "dashboard/DashboardController.hpp"

#include "../support/JobApplicationTestData.hpp"

#include <QtTest/QtTest>

#include <utility>

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

QString idAt(const QAbstractItemModel& model, int row)
{
    return model.data(model.index(row, 0), roleForName(model, "id")).toString();
}

CvDocument makeCv(QString id, int updateDay)
{
    CvDocument cv;
    cv.id_ = std::move(id);
    cv.originalFileName_ = QStringLiteral("%1.pdf").arg(cv.id_);
    cv.category_ = QStringLiteral("Engineering");
    cv.language_ = QStringLiteral("English");
    cv.createdAt_ = QDateTime{QDate{2026, 5, updateDay}, QTime{9, 0}, Qt::UTC};
    cv.updatedAt_ = cv.createdAt_;
    return cv;
}

} // namespace

class DashboardControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void statsModelAggregatesJobStatuses();
    void funnelModelExposesRatios();
    void recentModelsExposeBackendRows();
    void recentModelsSortNewestFirstAfterInsertionsAndUpdates();
};

void DashboardControllerTest::statsModelAggregatesJobStatuses()
{
    JobApplicationListModel applicationsModel(testsupport::makeJobApplications());
    CvListModel cvModel;
    DashboardController controller(applicationsModel, cvModel);
    const auto* model = controller.statsModel();

    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->data(model->index(0, 0), roleForName(*model, "title")).toString(), QStringLiteral("Total Jobs"));
    QCOMPARE(model->data(model->index(0, 0), roleForName(*model, "value")).toString(), QStringLiteral("6"));
    QCOMPARE(model->data(model->index(1, 0), roleForName(*model, "title")).toString(), QStringLiteral("Applied"));
    QCOMPARE(model->data(model->index(1, 0), roleForName(*model, "value")).toString(), QStringLiteral("1"));
    QCOMPARE(model->data(model->index(2, 0), roleForName(*model, "value")).toString(), QStringLiteral("2"));
    QCOMPARE(model->data(model->index(3, 0), roleForName(*model, "value")).toString(), QStringLiteral("1"));
    QCOMPARE(model->data(model->index(4, 0), roleForName(*model, "value")).toString(), QStringLiteral("4"));
}

void DashboardControllerTest::funnelModelExposesRatios()
{
    JobApplicationListModel applicationsModel(testsupport::makeJobApplications());
    CvListModel cvModel;
    DashboardController controller(applicationsModel, cvModel);
    const auto* model = controller.funnelModel();

    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->data(model->index(0, 0), roleForName(*model, "ratio")).toDouble(), 1.0);
    QCOMPARE(model->data(model->index(1, 0), roleForName(*model, "value")).toString(), QStringLiteral("1 (16.7%)"));
}

void DashboardControllerTest::recentModelsExposeBackendRows()
{
    JobApplicationListModel applicationsModel(testsupport::makeJobApplications());
    CvListModel cvModel;
    CvDocument cv;
    cv.id_ = QStringLiteral("cv-qt-2026");
    cv.originalFileName_ = QStringLiteral("CV_Qt_2026.pdf");
    cv.updatedAt_ = QDateTime{QDate{2026, 5, 12}, QTime{9, 0}, Qt::UTC};
    cv.linkedApplicationIds_ = {
        QStringLiteral("job-1"),
        QStringLiteral("job-2"),
        QStringLiteral("job-3"),
        QStringLiteral("job-4")};
    cvModel.appendDocument(cv);
    DashboardController controller(applicationsModel, cvModel);

    const auto* applications = controller.recentApplicationsModel();
    QCOMPARE(applications->rowCount(), 5);
    QCOMPARE(applications->data(applications->index(0, 0), roleForName(*applications, "jobTitle")).toString(), QStringLiteral("C++/Qt Developer"));
    QCOMPARE(applications->data(applications->index(0, 0), roleForName(*applications, "companyName")).toString(), QStringLiteral("KDAB"));
    QVERIFY(roleForName(*applications, "appliedDateLabel") > 0);

    const auto* cvs = controller.recentCvsModel();
    QCOMPARE(cvs->rowCount(), 1);
    QCOMPARE(cvs->data(cvs->index(0, 0), roleForName(*cvs, "fileName")).toString(), QStringLiteral("CV_Qt_2026.pdf"));
    QCOMPARE(cvs->data(cvs->index(0, 0), roleForName(*cvs, "linkedApplicationCountLabel")).toString(), QStringLiteral("4 jobs"));
}

void DashboardControllerTest::recentModelsSortNewestFirstAfterInsertionsAndUpdates()
{
    auto applications = testsupport::makeJobApplications();
    applications = {
        applications.at(2),
        applications.at(5),
        applications.at(0),
        applications.at(4),
        applications.at(1),
        applications.at(3),
    };
    JobApplicationListModel applicationsModel{applications};

    QVector<CvDocument> cvs{
        makeCv(QStringLiteral("cv-3"), 3),
        makeCv(QStringLiteral("cv-1"), 1),
        makeCv(QStringLiteral("cv-7"), 7),
        makeCv(QStringLiteral("cv-2"), 2),
        makeCv(QStringLiteral("cv-6"), 6),
        makeCv(QStringLiteral("cv-4"), 4),
        makeCv(QStringLiteral("cv-5"), 5),
    };
    CvListModel cvModel{cvs};
    DashboardController controller{applicationsModel, cvModel};

    const auto* recentApplications = controller.recentApplicationsModel();
    QCOMPARE(recentApplications->rowCount(), 5);
    QCOMPARE(idAt(*recentApplications, 0), QStringLiteral("job-kdab-cpp-qt"));
    QCOMPARE(idAt(*recentApplications, 4), QStringLiteral("job-byteworks-software"));

    auto insertedApplication = testsupport::makeJobApplication(
        QStringLiteral("job-newest"),
        QStringLiteral("company-newest"),
        QStringLiteral("Newest Company"),
        QStringLiteral("Newest Job"),
        QStringLiteral("cv-newest"),
        QStringLiteral("CV_Newest.pdf"),
        QDate{2026, 6, 1},
        JobStatus::Applied,
        QStringLiteral("Follow up"));
    applicationsModel.appendApplication(insertedApplication);
    applications.append(insertedApplication);
    QCOMPARE(idAt(*recentApplications, 0), QStringLiteral("job-newest"));

    applications[1].createdAt_ = QDateTime{QDate{2026, 6, 2}, QTime{10, 0}, Qt::UTC};
    applicationsModel.setApplications(applications);
    QCOMPARE(idAt(*recentApplications, 0), QStringLiteral("job-platforma-cpp"));
    QCOMPARE(recentApplications->rowCount(), 5);

    const auto* recentCvs = controller.recentCvsModel();
    QCOMPARE(recentCvs->rowCount(), 5);
    QCOMPARE(idAt(*recentCvs, 0), QStringLiteral("cv-7"));
    QCOMPARE(idAt(*recentCvs, 4), QStringLiteral("cv-3"));

    auto insertedCv = makeCv(QStringLiteral("cv-8"), 8);
    cvModel.appendDocument(insertedCv);
    QCOMPARE(idAt(*recentCvs, 0), QStringLiteral("cv-8"));

    QVERIFY(cvModel.setFavorite(
        QStringLiteral("cv-1"),
        true,
        QDateTime{QDate{2026, 6, 3}, QTime{10, 0}, Qt::UTC}));
    QCOMPARE(idAt(*recentCvs, 0), QStringLiteral("cv-1"));
    QCOMPARE(recentCvs->rowCount(), 5);
}

QTEST_APPLESS_MAIN(DashboardControllerTest)

#include "DashboardControllerTest.moc"

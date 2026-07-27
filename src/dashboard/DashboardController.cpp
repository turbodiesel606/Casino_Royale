#include "DashboardController.hpp"

#include <QLocale>

#include <utility>

namespace {

constexpr int recentRowLimit = 5;

struct ApplicationCounts
{
    int total_ = 0;
    int applied_ = 0;
    int interviews_ = 0;
    int rejected_ = 0;
    int active_ = 0;
};

QString percentageNote(int count, int total)
{
    if (total <= 0) {
        return QStringLiteral("0% of total");
    }

    return QStringLiteral("%1% of total").arg(QLocale::c().toString(100.0 * count / total, 'f', 1));
}

QString countWithPercent(int count, int total)
{
    if (total <= 0) {
        return QStringLiteral("%1 (0%)").arg(count);
    }

    return QStringLiteral("%1 (%2%)").arg(count).arg(QLocale::c().toString(100.0 * count / total, 'f', 1));
}

ApplicationCounts countApplications(const JobApplicationListModel& model)
{
    ApplicationCounts counts;
    counts.total_ = model.rowCount();

    for (int row = 0; row < model.rowCount(); ++row) {
        const auto* application = model.applicationAt(row);
        if (application == nullptr) {
            continue;
        }
        const auto status = application->status_;

        if (status == JobStatus::Applied) {
            ++counts.applied_;
        } else if (status == JobStatus::Interview) {
            ++counts.interviews_;
        } else if (status == JobStatus::Rejected) {
            ++counts.rejected_;
        }

        if (status != JobStatus::Rejected && status != JobStatus::Offer) {
            ++counts.active_;
        }
    }

    return counts;
}

DashboardMetric makeMetric(QString icon, QString title, QString value, QString note, double ratio, QString accent)
{
    DashboardMetric metric;
    metric.icon_ = std::move(icon);
    metric.title_ = std::move(title);
    metric.value_ = std::move(value);
    metric.note_ = std::move(note);
    metric.ratio_ = ratio;
    metric.accent_ = std::move(accent);
    return metric;
}

double ratio(int count, int total)
{
    return total > 0 ? static_cast<double>(count) / static_cast<double>(total) : 0.0;
}

QVector<DashboardMetric> makeStats(const JobApplicationListModel& model)
{
    const auto counts = countApplications(model);
    return {
        makeMetric(QStringLiteral("J"), QStringLiteral("Total Jobs"), QString::number(counts.total_), QStringLiteral("All time"), 1.0, QStringLiteral("#1687ff")),
        makeMetric(QStringLiteral(">"), QStringLiteral("Applied"), QString::number(counts.applied_), percentageNote(counts.applied_, counts.total_), ratio(counts.applied_, counts.total_), QStringLiteral("#2ecb68")),
        makeMetric(QStringLiteral("I"), QStringLiteral("Interviews"), QString::number(counts.interviews_), percentageNote(counts.interviews_, counts.total_), ratio(counts.interviews_, counts.total_), QStringLiteral("#ffbd21")),
        makeMetric(QStringLiteral("X"), QStringLiteral("Rejected"), QString::number(counts.rejected_), percentageNote(counts.rejected_, counts.total_), ratio(counts.rejected_, counts.total_), QStringLiteral("#ff4b49")),
        makeMetric(QStringLiteral("A"), QStringLiteral("Active"), QString::number(counts.active_), QStringLiteral("In progress"), ratio(counts.active_, counts.total_), QStringLiteral("#00bfd5")),
    };
}

QVector<DashboardMetric> makeFunnel(const JobApplicationListModel& model)
{
    const auto counts = countApplications(model);
    return {
        makeMetric(QString(), QStringLiteral("Total Jobs"), countWithPercent(counts.total_, counts.total_), QString(), 1.0, QStringLiteral("#167aff")),
        makeMetric(QString(), QStringLiteral("Applied"), countWithPercent(counts.applied_, counts.total_), QString(), ratio(counts.applied_, counts.total_), QStringLiteral("#2ecb68")),
        makeMetric(QString(), QStringLiteral("Interviews"), countWithPercent(counts.interviews_, counts.total_), QString(), ratio(counts.interviews_, counts.total_), QStringLiteral("#ffbb1d")),
        makeMetric(QString(), QStringLiteral("Rejected"), countWithPercent(counts.rejected_, counts.total_), QString(), ratio(counts.rejected_, counts.total_), QStringLiteral("#ff4b49")),
        makeMetric(QString(), QStringLiteral("Active"), countWithPercent(counts.active_, counts.total_), QString(), ratio(counts.active_, counts.total_), QStringLiteral("#00b8d4")),
    };
}

} // namespace

DashboardController::DashboardController(const JobApplicationListModel& applicationsModel, const CvListModel& cvModel, QObject* parent)
    : QObject(parent)
    , applicationsModel_(applicationsModel)
    , statsModel_(makeStats(applicationsModel), this)
    , funnelModel_(makeFunnel(applicationsModel), this)
    , recentApplicationsModel_(
        applicationsModel,
        JobApplicationListModel::CreatedAtRole,
        recentRowLimit,
        JobApplicationListModel::IdRole,
        Qt::DescendingOrder,
        this)
    , recentCvsModel_(
        cvModel,
        CvListModel::UpdatedAtRole,
        recentRowLimit,
        CvListModel::IdRole,
        Qt::DescendingOrder,
        this)
{
    recentApplicationsModel_.setRoleName(
        JobApplicationListModel::DateLabelRole,
        QByteArrayLiteral("appliedDateLabel"));
    connect(&applicationsModel_, &QAbstractItemModel::rowsInserted, this, &DashboardController::refreshMetrics);
    connect(&applicationsModel_, &QAbstractItemModel::modelReset, this, &DashboardController::refreshMetrics);
}

QAbstractItemModel* DashboardController::statsModel()
{
    return &statsModel_;
}

QAbstractItemModel* DashboardController::funnelModel()
{
    return &funnelModel_;
}

QAbstractItemModel* DashboardController::recentApplicationsModel()
{
    return &recentApplicationsModel_;
}

QAbstractItemModel* DashboardController::recentCvsModel()
{
    return &recentCvsModel_;
}

void DashboardController::refreshMetrics()
{
    statsModel_.setMetrics(makeStats(applicationsModel_));
    funnelModel_.setMetrics(makeFunnel(applicationsModel_));
}

#include "JobApplicationListModel.hpp"

#include "common/ModelPresentation.hpp"

#include <array>
#include <utility>

namespace {

	QString statusAccent(JobStatus status)
	{
		if (status == JobStatus::Interview) {
			return QStringLiteral("#ffbd21");
		}
		if (status == JobStatus::Offer) {
			return QStringLiteral("#38c86b");
		}
		if (status == JobStatus::Rejected) {
			return QStringLiteral("#ff4b49");
		}
		if (status == JobStatus::TestTask) {
			return QStringLiteral("#16c5dd");
		}
		return QStringLiteral("#c2c7cb");
	}

	QVariant roleValue(const JobApplication& application, int role)
	{
		switch (role) {
		case JobApplicationListModel::IdRole:
			return application.id_;
		case JobApplicationListModel::CompanyIdRole:
			return application.companyId_;
		case JobApplicationListModel::CompanyNameRole:
			return application.companyName_;
		case JobApplicationListModel::CompanyInitialsRole:
			return common::presentation::twoCharacterInitials(application.companyName_);
		case JobApplicationListModel::CompanyAccentRole:
			return common::presentation::companyAccent();
		case JobApplicationListModel::JobTitleRole:
			return application.jobTitle_;
		case JobApplicationListModel::JobUrlRole:
			return application.jobUrl_.toString();
		case JobApplicationListModel::WorkFormatRole:
			return workFormatToString(application.workFormat_);
		case JobApplicationListModel::CityRole:
			return application.city_;
		case JobApplicationListModel::SalaryRole:
			return application.salary_;
		case JobApplicationListModel::StatusRole:
		case JobApplicationListModel::StatusLabelRole:
			return jobStatusToString(application.status_);
		case JobApplicationListModel::StatusAccentRole:
			return statusAccent(application.status_);
		case JobApplicationListModel::AppliedDateRole:
			return application.appliedDate_.toString(Qt::ISODate);
		case JobApplicationListModel::DateLabelRole:
			return common::presentation::shortDateLabel(application.appliedDate_);
		case JobApplicationListModel::NextStepRole:
			return application.nextStep_;
		case JobApplicationListModel::CvIdRole:
			return application.cvId_;
		case JobApplicationListModel::CvFileNameRole:
			return application.cvFileName_;
		case JobApplicationListModel::DescriptionRole:
			return application.description_;
		case JobApplicationListModel::RequirementsRole:
			return application.requirements_;
		case JobApplicationListModel::TechStackRole:
			return application.techStack_;
		case JobApplicationListModel::NotesRole:
			return application.notes_;
		case JobApplicationListModel::StatusValueRole:
			return static_cast<int>(application.status_);
		case JobApplicationListModel::WorkFormatValueRole:
			return static_cast<int>(application.workFormat_);
		case JobApplicationListModel::AppliedDateValueRole:
			return application.appliedDate_;
		case JobApplicationListModel::CreatedAtRole:
			return application.createdAt_;
		case JobApplicationListModel::UpdatedAtRole:
			return application.updatedAt_;
		default:
			return {};
		}
	}

	QList<int> changedRoles(
		const JobApplication& previous,
		const JobApplication& current)
	{
		static constexpr std::array<int, 25> mutablePublishedRoles{
			JobApplicationListModel::CompanyIdRole,
			JobApplicationListModel::CompanyNameRole,
			JobApplicationListModel::CompanyInitialsRole,
			JobApplicationListModel::JobTitleRole,
			JobApplicationListModel::JobUrlRole,
			JobApplicationListModel::WorkFormatRole,
			JobApplicationListModel::WorkFormatValueRole,
			JobApplicationListModel::CityRole,
			JobApplicationListModel::SalaryRole,
			JobApplicationListModel::StatusRole,
			JobApplicationListModel::StatusLabelRole,
			JobApplicationListModel::StatusAccentRole,
			JobApplicationListModel::StatusValueRole,
			JobApplicationListModel::AppliedDateRole,
			JobApplicationListModel::DateLabelRole,
			JobApplicationListModel::AppliedDateValueRole,
			JobApplicationListModel::NextStepRole,
			JobApplicationListModel::CvIdRole,
			JobApplicationListModel::CvFileNameRole,
			JobApplicationListModel::DescriptionRole,
			JobApplicationListModel::RequirementsRole,
			JobApplicationListModel::TechStackRole,
			JobApplicationListModel::NotesRole,
			JobApplicationListModel::CreatedAtRole,
			JobApplicationListModel::UpdatedAtRole,
		};

		QList<int> roles;
		roles.reserve(mutablePublishedRoles.size());
		for (const auto role : mutablePublishedRoles) {
			if (roleValue(previous, role) != roleValue(current, role)) {
				roles.append(role);
			}
		}
		return roles;
	}

}

JobApplicationListModel::JobApplicationListModel(QObject* parent)
	: JobApplicationListModel(QVector<JobApplication>{}, parent)
{
}

JobApplicationListModel::JobApplicationListModel(QVector<JobApplication> applications, QObject* parent)
	: QAbstractListModel(parent)
	, applications_(std::move(applications))
{
}

int JobApplicationListModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid()) {
		return 0;
	}
	return applications_.size();
}

QVariant JobApplicationListModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= applications_.size()) {
		return {};
	}

	return roleValue(applications_.at(index.row()), role);
}

QHash<int, QByteArray> JobApplicationListModel::roleNames() const
{
	return {
		{IdRole, "id"},
		{CompanyIdRole, "companyId"},
		{CompanyNameRole, "companyName"},
		{CompanyInitialsRole, "companyInitials"},
		{CompanyAccentRole, "companyAccent"},
		{JobTitleRole, "jobTitle"},
		{JobUrlRole, "jobUrl"},
		{WorkFormatRole, "workFormat"},
		{CityRole, "city"},
		{SalaryRole, "salary"},
		{StatusRole, "status"},
		{StatusLabelRole, "statusLabel"},
		{StatusAccentRole, "statusAccent"},
		{AppliedDateRole, "appliedDate"},
		{DateLabelRole, "dateLabel"},
		{NextStepRole, "nextStep"},
		{CvIdRole, "cvId"},
		{CvFileNameRole, "cvFileName"},
		{DescriptionRole, "description"},
		{RequirementsRole, "requirements"},
		{TechStackRole, "techStack"},
		{NotesRole, "notes"},
		{StatusValueRole, "statusValue"},
		{WorkFormatValueRole, "workFormatValue"},
		{AppliedDateValueRole, "appliedDateValue"},
		{CreatedAtRole, "createdAt"},
		{UpdatedAtRole, "updatedAt"},
	};
}

const JobApplication* JobApplicationListModel::applicationAt(int row) const
{
	if (row < 0 || row >= applications_.size()) 
		return nullptr;
	
	return &applications_[row];
}

int JobApplicationListModel::rowForId(const QString& applicationId) const
{
	if (applicationId.isEmpty())
		return -1;
	
	for (int row = 0; row < applications_.size(); ++row) {
		if (applications_.at(row).id_ == applicationId) 
			return row;
	}
	return -1;
}

const JobApplication* JobApplicationListModel::applicationById(
	const QString& applicationId) const
{
	return applicationAt(rowForId(applicationId));
}

void JobApplicationListModel::setApplications(QVector<JobApplication> applications)
{
	beginResetModel();
	applications_ = std::move(applications);
	endResetModel();
}

void JobApplicationListModel::appendApplication(JobApplication application)
{
	const auto row = applications_.size();
	beginInsertRows({}, row, row);
	applications_.append(std::move(application));
	endInsertRows();
}

bool JobApplicationListModel::updateApplication(JobApplication application)
{
	const auto row = rowForId(application.id_);
	if (row < 0) {
		return false;
	}

	const auto roles = changedRoles(applications_.at(row), application);
	applications_[row] = std::move(application);
	if (!roles.isEmpty()) {
		const auto modelIndex = index(row, 0);
		emit dataChanged(modelIndex, modelIndex, roles);
	}
	return true;
}

int JobApplicationListModel::removeApplications(const QStringList& applicationIds)
{
	int removed = 0;
	for (int row = applications_.size() - 1; row >= 0; --row) {
		if (!applicationIds.contains(applications_.at(row).id_)) {
			continue;
		}
		beginRemoveRows({}, row, row);
		applications_.removeAt(row);
		endRemoveRows();
		++removed;
	}
	return removed;
}

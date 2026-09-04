#include "JobSaveWorker.hpp"

#include <QMetaType>

#include <type_traits>
#include <utility>
#include <variant>

JobSaveWorker::JobSaveWorker(
	QString dataDirectory,
	QObject* parent)
	: Base{
		QStringLiteral("JobSaveWorkerThread"),
		std::move(dataDirectory),
		parent }
{
	qRegisterMetaType<AddJobSaveOutcome>();
	qRegisterMetaType<UpdateJobSaveOutcome>();

	setOutcomeHandler(
		[this](Base::Outcome outcome) {
			std::visit(
				[this](const auto& value) {
					using OutcomeType =
						std::remove_cvref_t<decltype(value)>;

					if constexpr (
						std::is_same_v<OutcomeType, AddJobSaveOutcome>) {
						emit saveCompleted(value);
					}
					else if constexpr (
						std::is_same_v<OutcomeType, UpdateJobSaveOutcome>) {
						emit updateCompleted(value);
					}
				},
				outcome);
		});
}

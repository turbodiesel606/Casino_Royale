#include "CvImportWorker.hpp"

#include <QMetaType>

#include <utility>
#include <variant>

CvImportWorker::CvImportWorker(
	QString dataDirectory,
	QObject* parent)
	: Base{
		QStringLiteral("CvImportWorkerThread"),
		std::move(dataDirectory),
		parent }
{
	qRegisterMetaType<CvImportSaveOutcome>();

	setOutcomeHandler(
		[this](Base::Outcome outcome) {
			std::visit(
				[this](const auto& value) {
					emit importCompleted(value);
				},
				outcome);
		});
}

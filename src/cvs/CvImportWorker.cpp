#include "CvImportWorker.hpp"

#include <QMetaType>

#include <utility>
#include <variant>

CvImportWorker::CvImportWorker(
	QString dataDirectory,
	CvLockWrapper& cvMutationQueue,
	QObject* parent)
	: Base{
		QStringLiteral("CvImportWorkerThread"),
		[dataDirectory = std::move(dataDirectory), &cvMutationQueue] {
			return new CvImportExecutor{dataDirectory, cvMutationQueue};
		},
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

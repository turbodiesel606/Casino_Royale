#ifndef JOBTRACKER_SRC_CVS_CVIMPORTWORKERTYPES_HPP
#define JOBTRACKER_SRC_CVS_CVIMPORTWORKERTYPES_HPP

#include "CvDocument.hpp"
#include "CvImportService.hpp"
#include "common/CancellationState.hpp"

#include <QMetaType>
#include <QString>
#include <QUrl>
#include <QtGlobal>

#include <memory>

// Carries one CV Library import request into the worker thread.
struct CvImportRequest final
{
	quint64 operationId_ = 0;
	QUrl sourceUrl_;
	std::shared_ptr<CancellationState> cancellation_;
};

// Returns one value-only import outcome to the GUI thread.
struct CvImportSaveOutcome final
{
	quint64 operationId_ = 0;
	std::shared_ptr<CancellationState> cancellation_;
	QString fileName_;
	CvDocument document_;
	QString message_;
	bool success_ = false;
	CvImportDisposition disposition_ = CvImportDisposition::ExistingActive;
};

Q_DECLARE_METATYPE(CvImportSaveOutcome)

#endif // JOBTRACKER_SRC_CVS_CVIMPORTWORKERTYPES_HPP

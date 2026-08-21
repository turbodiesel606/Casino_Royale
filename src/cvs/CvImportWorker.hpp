#ifndef JOBTRACKER_SRC_CVS_CVIMPORTWORKER_HPP
#define JOBTRACKER_SRC_CVS_CVIMPORTWORKER_HPP

#include "CvDocument.hpp"
#include "CvImportService.hpp"
#include "common/CancellationState.hpp"

#include <QMetaType>
#include <QObject>
#include <QString>
#include <QThread>
#include <QUrl>

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

// GUI-thread facade for one reusable CV import worker thread and its private
// persistence context. The controller submits at most one active request.
class CvImportWorker final : public QObject
{
    Q_OBJECT

public:
    explicit CvImportWorker(QString dataDirectory, QObject* parent = nullptr);
    ~CvImportWorker() override;

    void submit(CvImportRequest request);
    void shutdown();
    bool isRunning() const;

signals:
    void importCompleted(const CvImportSaveOutcome& outcome);

private:
    class Executor;

    void deliverImportOutcome(CvImportSaveOutcome outcome);
    void queueUnavailableOutcome(const CvImportRequest& request, const QString& message);
    bool isActiveOutcome(
        quint64 operationId,
        const std::shared_ptr<CancellationState>& cancellation) const;
    void clearActiveRequest();

    QString dataDirectory_;
    QThread workerThread_;
    Executor* executor_ = nullptr;
    std::shared_ptr<CancellationState> activeCancellation_;
    quint64 activeOperationId_ = 0;
    bool busy_ = false;
    bool shuttingDown_ = false;
};

#endif // JOBTRACKER_SRC_CVS_CVIMPORTWORKER_HPP

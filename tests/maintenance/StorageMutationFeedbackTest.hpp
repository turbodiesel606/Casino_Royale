#ifndef JOBTRACKER_TESTS_MAINTENANCE_STORAGEMUTATIONFEEDBACKTEST_HPP
#define JOBTRACKER_TESTS_MAINTENANCE_STORAGEMUTATIONFEEDBACKTEST_HPP

#include <QObject>

class StorageMutationFeedbackTest final : public QObject
{
    Q_OBJECT

private slots:
    void repeatedAttemptsReopenDialogAndBannerStaysCurrent();
    void pendingWorkDialogReportsOnlyExplicitCloseChoices();
    void bootstrapOpensAndExits_data();
    void bootstrapOpensAndExits();
    void shellInteractions_data();
    void shellInteractions();
};

#endif

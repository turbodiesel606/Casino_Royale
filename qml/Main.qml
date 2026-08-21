import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "components"
import "pages"

ApplicationWindow {
    id: window
    width: 1660
    height: 948
    minimumWidth: 1180
    minimumHeight: 720
    visible: true
    title: "JobTracker"
    color: "#07131d"

    property int currentPage: 0
    property int previousPage: 0
    property bool jobFormVisible: false
    property var notifications: []
    property var currentNotification: null
    property bool notificationsPaused: false
    property bool exitAfterPendingWorkDrained: false
    property var pendingProtectedAction: null
    property bool allowUnsavedCloseOnce: false

    function performOpenJobForm() {
        window.previousPage = window.currentPage >= 0 ? window.currentPage : 0
        window.currentPage = -1
        window.jobFormVisible = true
    }

    function openJobForm() {
        requestProtectedAction("openJobForm", ({}))
    }

    function closeJobForm() {
        window.jobFormVisible = false
        window.currentPage = window.previousPage
    }

    function performProtectedAction(action) {
        if (!action)
            return

        jobsPage.exitCleanJobEditMode()
        const payload = action.payload || ({})
        if (action.kind === "navigate") {
            window.currentPage = payload.index
            window.jobFormVisible = false
        } else if (action.kind === "openJobForm") {
            window.performOpenJobForm()
        } else if (action.kind === "openCvPicker") {
            cvFileDialog.open()
        } else if (action.kind === "showJobApplications") {
            jobsPage.showApplications()
        } else if (action.kind === "closeApplication") {
            window.allowUnsavedCloseOnce = true
            Qt.callLater(function() { window.close() })
        } else if (typeof payload.execute === "function") {
            payload.execute()
        }
    }

    function requestProtectedAction(kind, payload) {
        if (window.pendingProtectedAction !== null)
            return

        const action = { kind: kind, payload: payload || ({}) }
        if (jobsPage.jobUpdateInProgress) {
            window.pendingProtectedAction = action
            return
        }
        if (jobsPage.hasUnsavedJobChanges) {
            window.pendingProtectedAction = action
            unsavedChangesDialog.open()
            return
        }

        window.performProtectedAction(action)
    }

    function saveUnsavedChangesAndContinue() {
        unsavedChangesDialog.close()
        jobsPage.savePendingJobChanges()
    }

    function discardUnsavedChangesAndContinue() {
        const action = window.pendingProtectedAction
        window.pendingProtectedAction = null
        unsavedChangesDialog.close()
        jobsPage.discardPendingJobChanges()
        window.performProtectedAction(action)
    }

    function cancelProtectedAction() {
        window.pendingProtectedAction = null
        unsavedChangesDialog.close()
    }

    function pendingWorkCount() {
        return jobApplicationsController.pendingSaveCount
            + cvLibraryController.pendingImportCount
            + jobApplicationsController.pendingDeletionCount
            + cvLibraryController.pendingDeletionCount
    }

    function enqueueNotification(title, severity, message) {
        if (exitAfterPendingWorkDrained)
            return

        notifications = notifications.concat([{
            title: title,
            severity: severity,
            message: message
        }])
        showNextNotification()
    }

    function showNextNotification() {
        if (notificationsPaused
                || exitAfterPendingWorkDrained
                || notificationPopup.visible
                || notifications.length === 0) {
            return
        }

        currentNotification = notifications[0]
        notifications = notifications.slice(1)
        notificationPopup.open()
    }

    function pauseNotifications() {
        if (notificationsPaused)
            return

        notificationsPaused = true
        notificationTimer.stop()
        if (notificationPopup.visible && currentNotification) {
            notifications = [currentNotification].concat(notifications)
            notificationPopup.close()
        }
    }

    function resumeNotifications() {
        notificationsPaused = false
        Qt.callLater(function() { window.showNextNotification() })
    }

    function discardNotifications() {
        notificationsPaused = true
        notifications = []
        notificationTimer.stop()
        notificationPopup.close()
    }

    function waitForPendingWork() {
        closeConfirmation.close()
        resumeNotifications()
    }

    function finishCloseIfWorkDrained() {
        if (pendingWorkCount() !== 0)
            return

        if (exitAfterPendingWorkDrained) {
            Qt.callLater(function() { window.close() })
        } else if (closeConfirmation.visible) {
            closeConfirmation.close()
            resumeNotifications()
        }
    }

    function interruptPendingWorkAndExit() {
        exitAfterPendingWorkDrained = true
        discardNotifications()
        closeConfirmation.close()
        jobApplicationsController.cancelAllJobSaves()
        cvLibraryController.cancelAllCvImports()
        jobApplicationsController.cancelApplicationDeletion()
        cvLibraryController.cancelCvMutation()
        finishCloseIfWorkDrained()
    }

    onClosing: function(close) {
        if (window.allowUnsavedCloseOnce) {
            window.allowUnsavedCloseOnce = false
        } else if (jobsPage.jobUpdateInProgress
                   || jobsPage.hasUnsavedJobChanges) {
            close.accepted = false
            window.requestProtectedAction("closeApplication", ({}))
            return
        } else {
            jobsPage.exitCleanJobEditMode()
        }

        if (window.pendingWorkCount() > 0) {
            close.accepted = false
            if (!exitAfterPendingWorkDrained && !closeConfirmation.visible) {
                pauseNotifications()
                closeConfirmation.open()
            }
        }
    }

    Connections {
        target: jobApplicationsController

        function onApplicationSaveCompleted(operationId, jobTitle, success, message) {
            window.enqueueNotification(
                success ? "Saved: " + jobTitle : "Save failed: " + jobTitle,
                success ? "success" : "error",
                message)
        }

        function onPendingSaveCountChanged() {
            window.finishCloseIfWorkDrained()
        }

        function onApplicationUpdateRejected(operationId, applicationId, errors, message) {
            window.pendingProtectedAction = null
            window.enqueueNotification(
                "Update rejected",
                "error",
                message)
        }

        function onApplicationUpdateCompleted(operationId, applicationId, jobTitle,
                                              success, errors, message) {
            window.enqueueNotification(
                success ? "Updated: " + jobTitle : "Update failed: " + jobTitle,
                success ? "success" : "error",
                message)

            const action = window.pendingProtectedAction
            window.pendingProtectedAction = null
            if (success && action) {
                Qt.callLater(function() {
                    window.performProtectedAction(action)
                })
            }
        }

        function onApplicationDeletionCompleted(deletedCount, failedCount, cancelled, message) {
            const severity = failedCount > 0 ? "error" : (cancelled ? "warning" : "success")
            window.enqueueNotification(
                "Job deletion complete: " + deletedCount + " deleted",
                severity,
                message)
        }

        function onPendingDeletionCountChanged() {
            window.finishCloseIfWorkDrained()
        }
    }

    Connections {
        target: cvLibraryController

        function onCvImportCompleted(operationId, fileName, success, disposition, message) {
            const added = disposition === "inserted"
            const restored = disposition === "restored-archived"
            const severity = !success ? "error" : ((added || restored) ? "success" : "warning")
            const title = !success
                ? "CV upload failed: " + fileName
                : (added
                    ? "CV added: " + fileName
                    : (restored
                        ? "CV restored: " + fileName
                        : "CV already exists: " + fileName))
            window.enqueueNotification(title, severity, message)
        }

        function onPendingImportCountChanged() {
            window.finishCloseIfWorkDrained()
        }

        function onCvMutationCompleted(deletedCount, archivedCount, restoredCount, skippedCount, failedCount, cancelled, message) {
            const severity = failedCount > 0 ? "error" : ((skippedCount > 0 || cancelled) ? "warning" : "success")
            window.enqueueNotification(
                "CV operation complete",
                severity,
                message)
        }

        function onPendingDeletionCountChanged() {
            window.finishCloseIfWorkDrained()
        }
    }

    FileDialog {
        id: cvFileDialog
        title: "Add CVs"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["CV documents (*.pdf *.doc *.docx)"]
        onAccepted: cvLibraryController.addCvs(selectedFiles)
    }

    Timer {
        id: notificationTimer
        interval: 15000
        repeat: false
        onTriggered: notificationPopup.close()
    }

    Popup {
        id: notificationPopup
        x: window.width - width - 24
        y: 24
        width: Math.min(430, window.width - 48)
        padding: 18
        modal: false
        focus: false
        closePolicy: Popup.NoAutoClose
        z: 900

        onOpened: notificationTimer.restart()
        onClosed: {
            notificationTimer.stop()
            window.currentNotification = null
            if (!window.notificationsPaused && !window.exitAfterPendingWorkDrained)
                Qt.callLater(function() { window.showNextNotification() })
        }

        background: Rectangle {
            color: "#102330"
            border.color: {
                if (!window.currentNotification)
                    return "#2e4657"
                if (window.currentNotification.severity === "success")
                    return "#42d392"
                if (window.currentNotification.severity === "warning")
                    return "#ffbd21"
                return "#ff6b69"
            }
            border.width: 1
            radius: 8
        }

        contentItem: ColumnLayout {
            spacing: 7

            Text {
                Layout.fillWidth: true
                text: window.currentNotification ? window.currentNotification.title : ""
                color: "#eef3f8"
                font.pixelSize: 16
                font.bold: true
                wrapMode: Text.Wrap
            }

            Text {
                Layout.fillWidth: true
                text: window.currentNotification
                    ? window.currentNotification.message
                    : ""
                color: "#c1ccd6"
                font.pixelSize: 14
                wrapMode: Text.Wrap
            }
        }
    }

    Dialog {
        id: unsavedChangesDialog
        x: Math.round((window.width - width) / 2)
        y: Math.round((window.height - height) / 2)
        width: Math.min(590, window.width - 48)
        modal: true
        focus: true
        closePolicy: Popup.NoAutoClose
        title: "Unsaved changes"
        z: 1100

        contentItem: Text {
            text: "You have unsaved changes in the Job Description. What would you like to do?"
            color: "#eef3f8"
            font.pixelSize: 15
            wrapMode: Text.Wrap
        }

        footer: DialogButtonBox {
            PrimaryButton {
                text: "Cancel"
                subtle: true
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                onClicked: window.cancelProtectedAction()
            }

            DangerButton {
                text: "Don't Save"
                DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
                onClicked: window.discardUnsavedChangesAndContinue()
            }

            PrimaryButton {
                text: "Save"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: window.saveUnsavedChangesAndContinue()
            }
        }

        background: Rectangle {
            color: "#102330"
            border.color: "#2e4657"
            border.width: 1
            radius: 8
        }
    }

    Dialog {
        id: closeConfirmation
        x: Math.round((window.width - width) / 2)
        y: Math.round((window.height - height) / 2)
        width: Math.min(620, window.width - 48)
        modal: true
        focus: true
        closePolicy: Popup.NoAutoClose
        title: "Work in progress"
        z: 1000

        contentItem: Text {
            text: {
                const pendingJobs = jobApplicationsController.pendingSaveCount > 0
                const pendingCvs = cvLibraryController.pendingImportCount > 0
                const deleting = jobApplicationsController.pendingDeletionCount > 0
                    || cvLibraryController.pendingDeletionCount > 0
                if (deleting)
                    return "Items are still being deleted or archived. Do you want to wait or interrupt after the current item and exit? Already completed items will remain committed."
                if (pendingJobs && pendingCvs)
                    return "Job applications and CVs are still being saved. Do you want to wait or interrupt the pending work and exit?"
                if (pendingCvs)
                    return "One or more CVs are still being added. Do you want to wait or interrupt the imports and exit?"
                return "A job application is still being saved. Do you want to wait or interrupt the save and exit?"
            }
            color: "#eef3f8"
            font.pixelSize: 16
            wrapMode: Text.Wrap
        }

        footer: DialogButtonBox {
            PrimaryButton {
                text: "Wait"
                subtle: true
                DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
                onClicked: window.waitForPendingWork()
            }

            DangerButton {
                text: "Interrupt and Exit"
                DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
                onClicked: window.interruptPendingWorkAndExit()
            }
        }

        background: Rectangle {
            color: "#102330"
            border.color: "#2e4657"
            border.width: 1
            radius: 8
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            Layout.fillHeight: true
            Layout.preferredWidth: 242
            currentIndex: window.jobFormVisible ? -1 : window.currentPage
            actionsEnabled: jobApplicationsController.pendingDeletionCount === 0
                && cvLibraryController.pendingDeletionCount === 0
            onNavigate: index => {
                window.requestProtectedAction("navigate", { index: index })
            }
            onAddJob: window.openJobForm()
            onAddCv: window.requestProtectedAction("openCvPicker", ({}))
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.jobFormVisible ? 5 : Math.max(window.currentPage, 0)

            DashboardPage { onAddJobRequested: window.openJobForm() }
            JobsPage {
                id: jobsPage
                onAddJobRequested: window.openJobForm()
                onProtectedActionRequested: function(kind, payload) {
                    window.requestProtectedAction(kind, payload)
                }
            }
            CvLibraryPage { }
            CompaniesPage { }
            ContactsPage { }
            JobFormPage { }
        }
    }
}

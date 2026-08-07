import QtQuick
import QtQuick.Controls
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
    property var saveNotifications: []
    property var currentSaveNotification: null
    property bool saveNotificationsPaused: false
    property bool exitAfterSaveQueueDrained: false

    function openJobForm() {
        window.previousPage = window.currentPage >= 0 ? window.currentPage : 0
        window.currentPage = -1
        window.jobFormVisible = true
    }

    function closeJobForm() {
        window.jobFormVisible = false
        window.currentPage = window.previousPage
    }

    function enqueueSaveNotification(jobTitle, success, message) {
        saveNotifications = saveNotifications.concat([{
            jobTitle: jobTitle,
            success: success,
            message: message
        }])
        showNextSaveNotification()
    }

    function showNextSaveNotification() {
        if (saveNotificationsPaused
                || exitAfterSaveQueueDrained
                || saveNotificationPopup.visible
                || saveNotifications.length === 0) {
            return
        }

        currentSaveNotification = saveNotifications[0]
        saveNotifications = saveNotifications.slice(1)
        saveNotificationPopup.open()
    }

    function pauseSaveNotifications() {
        if (saveNotificationsPaused)
            return

        saveNotificationsPaused = true
        notificationTimer.stop()
        if (saveNotificationPopup.visible && currentSaveNotification) {
            saveNotifications = [currentSaveNotification].concat(saveNotifications)
            saveNotificationPopup.close()
        }
    }

    function resumeSaveNotifications() {
        saveNotificationsPaused = false
        Qt.callLater(function() { window.showNextSaveNotification() })
    }

    function discardSaveNotifications() {
        saveNotificationsPaused = true
        saveNotifications = []
        notificationTimer.stop()
        saveNotificationPopup.close()
    }

    function waitForPendingSaves() {
        closeConfirmation.close()
        resumeSaveNotifications()
    }

    function interruptPendingSavesAndExit() {
        exitAfterSaveQueueDrained = true
        discardSaveNotifications()
        closeConfirmation.close()
        jobApplicationsController.cancelAllCreateApplications()
    }

    onClosing: function(close) {
        if (jobApplicationsController.pendingSaveCount > 0) {
            close.accepted = false
            if (!exitAfterSaveQueueDrained && !closeConfirmation.visible) {
                pauseSaveNotifications()
                closeConfirmation.open()
            }
        }
    }

    Connections {
        target: jobApplicationsController

        function onApplicationSaveCompleted(operationId, jobTitle, success, message) {
            window.enqueueSaveNotification(jobTitle, success, message)
        }

        function onPendingSaveCountChanged() {
            if (jobApplicationsController.pendingSaveCount === 0
                    && closeConfirmation.visible
                    && !window.exitAfterSaveQueueDrained) {
                closeConfirmation.close()
                window.resumeSaveNotifications()
            }
        }

        function onSaveQueueDrained() {
            if (window.exitAfterSaveQueueDrained) {
                Qt.callLater(function() { window.close() })
            }
        }
    }

    Timer {
        id: notificationTimer
        interval: 15000
        repeat: false
        onTriggered: saveNotificationPopup.close()
    }

    Popup {
        id: saveNotificationPopup
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
            window.currentSaveNotification = null
            if (!window.saveNotificationsPaused && !window.exitAfterSaveQueueDrained)
                Qt.callLater(function() { window.showNextSaveNotification() })
        }

        background: Rectangle {
            color: "#102330"
            border.color: window.currentSaveNotification && window.currentSaveNotification.success
                ? "#42d392"
                : "#ff6b69"
            border.width: 1
            radius: 8
        }

        contentItem: ColumnLayout {
            spacing: 7

            Text {
                Layout.fillWidth: true
                text: {
                    if (!window.currentSaveNotification)
                        return ""
                    return window.currentSaveNotification.success
                        ? "Saved: " + window.currentSaveNotification.jobTitle
                        : "Save failed: " + window.currentSaveNotification.jobTitle
                }
                color: "#eef3f8"
                font.pixelSize: 16
                font.bold: true
                wrapMode: Text.Wrap
            }

            Text {
                Layout.fillWidth: true
                text: window.currentSaveNotification
                    ? window.currentSaveNotification.message
                    : ""
                color: "#c1ccd6"
                font.pixelSize: 14
                wrapMode: Text.Wrap
            }
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
        title: "Save in progress"
        z: 1000

        contentItem: Text {
            text: "\u201cA job application is currently being saved. Are you sure you want to interrupt the save operation?\u201d"
            color: "#eef3f8"
            font.pixelSize: 16
            wrapMode: Text.Wrap
        }

        footer: DialogButtonBox {
            Button {
                text: "Wait"
                DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
                onClicked: window.waitForPendingSaves()
            }

            Button {
                text: "Interrupt and Exit"
                DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
                onClicked: window.interruptPendingSavesAndExit()
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
            onNavigate: index => {
                window.currentPage = index
                window.jobFormVisible = false
            }
            onAddJob: window.openJobForm()
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.jobFormVisible ? 5 : Math.max(window.currentPage, 0)

            DashboardPage { onAddJobRequested: window.openJobForm() }
            JobsPage { onAddJobRequested: window.openJobForm() }
            CvLibraryPage { }
            CompaniesPage { }
            ContactsPage { }
            JobFormPage { }
        }
    }
}

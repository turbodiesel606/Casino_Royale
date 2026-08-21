import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page

    signal addJobRequested()
    signal protectedActionRequested(string kind, var payload)

    property bool descriptionMode: false

    readonly property bool hasUnsavedJobChanges: descriptionPane.editMode
        && descriptionPane.hasUnsavedChanges
    readonly property bool jobUpdateInProgress: descriptionPane.saveInProgress
    readonly property bool jobDescriptionEditMode: descriptionPane.editMode

    readonly property var selectedApplication: jobApplicationsController.selectedApplication

    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property color panelColor: "#0c1d29"
    readonly property color panelLineColor: "#223542"
    readonly property color blueColor: "#1687ff"

    readonly property int tableHeaderHeight: 44
    readonly property int tableRowHeight: 49
    readonly property int tableFooterHeight: 68

    readonly property var columns: [
        { "title": "Company", "x": 52, "width": 145 },
        { "title": "CV Used", "x": 197, "width": 132 },
        { "title": "Date", "x": 329, "width": 105 },
        { "title": "Job Title", "x": 442, "width": 160 },
        { "title": "Status", "x": 610, "width": 105 },
        { "title": "Applied", "x": 723, "width": 100 },
        { "title": "Next Step", "x": 831, "width": 130 }
    ]

    readonly property var previewDetails: [
        ["CV", "CV used", page.selectedApplication.cvFileName],
        ["WF", "Format", page.selectedApplication.workFormat],
        ["$", "Salary", page.selectedApplication.salary],
        ["ST", "Status", page.selectedApplication.statusLabel],
        ["AD", "Applied", page.selectedApplication.appliedDate],
        ["NS", "Next Step", page.selectedApplication.nextStep]
    ]

    function savePendingJobChanges() {
        descriptionPane.submitUpdate()
    }

    function discardPendingJobChanges() {
        descriptionPane.discardEdits()
    }

    function exitCleanJobEditMode() {
        descriptionPane.exitCleanEditMode()
    }

    function showApplications() {
        descriptionPane.exitCleanEditMode()
        page.descriptionMode = false
    }

    JobsApplicationsPane {
        anchors.fill: parent
        selectedRow: jobApplicationsController.selectedApplicationIndex
        textColor: page.textColor
        mutedColor: page.mutedColor
        panelColor: page.panelColor
        panelLineColor: page.panelLineColor
        blueColor: page.blueColor
        tableHeaderHeight: page.tableHeaderHeight
        tableRowHeight: page.tableRowHeight
        tableFooterHeight: page.tableFooterHeight
        applicationsModel: jobApplicationsController.applicationsModel
        applicationCount: jobApplicationsController.applicationCount
        columns: page.columns
        previewDetails: page.previewDetails
        resultSummary: jobApplicationsController.resultSummary
        searchText: jobApplicationsController.searchText
        statusFilter: jobApplicationsController.statusFilter
        previewTitle: page.selectedApplication.jobTitle
        previewCompany: page.selectedApplication.companyName
        previewCompanyInitials: page.selectedApplication.companyInitials
        previewCompanyAccent: page.selectedApplication.companyAccent
        previewNotes: page.selectedApplication.notes
        previewStatusAccent: page.selectedApplication.statusAccent
        checkedApplicationIds: jobApplicationsController.checkedApplicationIds
        allVisibleApplicationsChecked: jobApplicationsController.allVisibleApplicationsChecked
        someVisibleApplicationsChecked: jobApplicationsController.someVisibleApplicationsChecked
        deleteEnabled: jobApplicationsController.canDeleteApplications
        deletionBusy: jobApplicationsController.deletingApplications
        onRowSelected: row => jobApplicationsController.selectApplication(row)
        onSearchRequested: text => jobApplicationsController.setSearchText(text)
        onStatusFilterRequested: status => jobApplicationsController.setStatusFilter(status)
        onApplicationsRequested: page.descriptionMode = false
        onDescriptionRequested: page.descriptionMode = true
        onRowCheckToggled: row => jobApplicationsController.toggleApplicationChecked(row)
        onAllVisibleCheckedRequested: checked => jobApplicationsController.setAllVisibleApplicationsChecked(checked)
        onDeleteRequested: deleteConfirmation.open()
    }

    JobDescriptionPane {
        id: descriptionPane
        anchors.fill: parent
        visible: page.descriptionMode
        textColor: page.textColor
        mutedColor: page.mutedColor
        panelLineColor: page.panelLineColor
        blueColor: page.blueColor
        selectedApplication: page.selectedApplication
        onApplicationsRequested: page.protectedActionRequested(
            "showJobApplications", ({}))
    }

    DestructiveConfirmationDialog {
        id: deleteConfirmation
        anchors.centerIn: parent
        title: "Delete job applications"
        confirmText: "Delete Selected"
        message: "Delete " + jobApplicationsController.checkedApplicationCount
            + " selected job application(s)? Their CVs will remain in the CV Library."
        onConfirmed: jobApplicationsController.deleteCheckedApplications()
    }
}

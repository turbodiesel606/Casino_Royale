import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page

    signal addJobRequested()

    property bool descriptionMode: false

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
        { "title": "Company", "x": 18, "width": 160 },
        { "title": "CV Used", "x": 178, "width": 145 },
        { "title": "Date", "x": 320, "width": 118 },
        { "title": "Job Title", "x": 452, "width": 175 },
        { "title": "Status", "x": 635, "width": 118 },
        { "title": "Applied", "x": 756, "width": 112 },
        { "title": "Next Step", "x": 876, "width": 140 }
    ]

    readonly property var previewDetails: [
        ["CV", "CV used", page.selectedApplication.cvFileName],
        ["WF", "Format", page.selectedApplication.workFormat],
        ["$", "Salary", page.selectedApplication.salary],
        ["ST", "Status", page.selectedApplication.statusLabel],
        ["AD", "Applied", page.selectedApplication.appliedDate],
        ["NS", "Next Step", page.selectedApplication.nextStep]
    ]

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
        onRowSelected: row => jobApplicationsController.selectApplication(row)
        onSearchRequested: text => jobApplicationsController.setSearchText(text)
        onStatusFilterRequested: status => jobApplicationsController.setStatusFilter(status)
        onApplicationsRequested: page.descriptionMode = false
        onDescriptionRequested: page.descriptionMode = true
    }

    JobDescriptionPane {
        anchors.fill: parent
        visible: page.descriptionMode
        textColor: page.textColor
        mutedColor: page.mutedColor
        panelLineColor: page.panelLineColor
        blueColor: page.blueColor
        selectedApplication: page.selectedApplication
        onApplicationsRequested: page.descriptionMode = false
    }
}

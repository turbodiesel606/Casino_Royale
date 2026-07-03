import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page

    signal addJobRequested()

    property int selectedRow: 0
    property bool descriptionMode: false

    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property color panelColor: "#0c1d29"
    readonly property color panelLineColor: "#223542"
    readonly property color blueColor: "#1687ff"

    readonly property int tableHeaderHeight: 44
    readonly property int tableRowHeight: 49
    readonly property int tableFooterHeight: 68

    readonly property var applications: [
        ["KDAB", "CV_Qt_2026.pdf", "May 12, 2026", "C++/Qt Developer", "Applied", "May 12, 2026", "Test Task", "#146ce0", "KDAB"],
        ["TechSoft", "CV_Qt_2026.pdf", "May 8, 2026", "Qt/QML Engineer", "Interview", "May 8, 2026", "Interview", "#153046", "Te"],
        ["Vision Systems", "CV_Embedded.pdf", "May 5, 2026", "Embedded Developer", "Test Task", "May 5, 2026", "Test Task", "#153046", "Vi"],
        ["GreenWidget", "CV_General.pdf", "Apr 28, 2026", "Software Engineer", "Offer", "Apr 28, 2026", "Salary Discussion", "#153046", "Gr"],
        ["CodeCraft", "CV_Qt_2026.pdf", "Apr 22, 2026", "C++/Qt Developer", "Interview", "Apr 22, 2026", "Technical Interview", "#153046", "Co"],
        ["Nexora", "CV_Backend.pdf", "Apr 18, 2026", "Backend Developer", "Applied", "Apr 18, 2026", "Screening Call", "#153046", "Ne"],
        ["ByteWorks", "CV_General.pdf", "Apr 10, 2026", "Software Engineer", "Rejected", "Apr 10, 2026", "—", "#153046", "By"],
        ["Innotech", "CV_Qt_2026.pdf", "Apr 2, 2026", "Qt/QML Engiqneer", "Test Task", "Apr 2, 2026", "Test Task", "#153046", "In"],
        ["Platforma", "CV_General.pdf", "Mar 28, 2026", "C++ Developer", "Interview", "Mar 28, 2026", "HR Interview", "#153046", "Pl"],
        ["DevSolutions", "CV_Embedded.pdf", "Mar 20, 2026", "Embedded C++ Engineer", "Applied", "Mar 20, 2026", "Screening Call", "#153046", "De"]
    ]

    readonly property var columns: [
        { "title": "Company", "x": 18, "width": 160 },
        { "title": "CV Used", "x": 178, "width": 145 },
        { "title": "Date ↓", "x": 320, "width": 118 },
        { "title": "Job Title", "x": 452, "width": 175 },
        { "title": "Status", "x": 635, "width": 118 },
        { "title": "Applied", "x": 756, "width": 112 },
        { "title": "Next Step", "x": 876, "width": 140 }
    ]

    readonly property var previewDetails: [
        ["♕", "CV used", page.applications[page.selectedRow][1]],
        ["⇄", "Format", "Remote"],
        ["⌁", "Salary", "$4,500"],
        ["◷", "Status", page.applications[page.selectedRow][4]],
        ["▣", "Applied", page.applications[page.selectedRow][5]],
        ["⚑", "Next Step", page.applications[page.selectedRow][6]]
    ]

    JobsApplicationsPane {
        anchors.fill: parent
        selectedRow: page.selectedRow
        textColor: page.textColor
        mutedColor: page.mutedColor
        panelColor: page.panelColor
        panelLineColor: page.panelLineColor
        blueColor: page.blueColor
        tableHeaderHeight: page.tableHeaderHeight
        tableRowHeight: page.tableRowHeight
        tableFooterHeight: page.tableFooterHeight
        applications: page.applications
        columns: page.columns
        previewDetails: page.previewDetails
        onRowSelected: row => page.selectedRow = row
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
        onApplicationsRequested: page.descriptionMode = false
    }
}

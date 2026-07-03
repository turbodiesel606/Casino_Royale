import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page
    clip: true

    property int selectedRow: 0
    readonly property real uiScale: 0.80
    readonly property color bgColor: "#07131d"
    readonly property color panelColor: "#0b1b27"
    readonly property color panelSoftColor: "#0e202d"
    readonly property color lineColor: "#243746"
    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property color blueColor: "#1687ff"
    readonly property color greenColor: "#59d34d"
    readonly property color yellowColor: "#ffbd21"
    readonly property color purpleColor: "#b36bff"
    readonly property var cvs: [
        {
            file: "CV_Qt_2026.pdf",
            title: "Qt/QML Engineer",
            category: "Qt/QML Developer",
            language: "English",
            updated: "May 12, 2026",
            jobs: "8",
            size: "612 KB",
            description: "CV focused on Qt/QML development, desktop applications, and cross-platform experience."
        },
        {
            file: "CV_Cpp_Developer.pdf",
            title: "C++ Developer",
            category: "C++ Developer",
            language: "English",
            updated: "May 5, 2026",
            jobs: "5",
            size: "548 KB",
            description: "CV focused on C++ desktop engineering, Qt Widgets, and modern CMake projects."
        },
        {
            file: "CV_Remote.pdf",
            title: "Remote Software Engineer",
            category: "English CV",
            language: "English",
            updated: "Apr 28, 2026",
            jobs: "3",
            size: "584 KB",
            description: "Remote-first CV for distributed software teams and cross-platform product work."
        },
        {
            file: "CV_Office.pdf",
            title: "Office Administrator",
            category: "Office Job",
            language: "English",
            updated: "Apr 18, 2026",
            jobs: "2",
            size: "470 KB",
            description: "Office administration CV focused on operations, documentation, and communication."
        }
    ]
    readonly property var selectedCv: cvs[selectedRow]

    function categoryColor(category) {
        if (category === "Office Job")
            return page.yellowColor
        if (category === "English CV")
            return page.purpleColor
        return page.blueColor
    }

    Item {
        id: scaledContent
        width: page.width / page.uiScale
        height: page.height / page.uiScale
        scale: page.uiScale
        transformOrigin: Item.TopLeft

        Rectangle {
            anchors.fill: parent
            color: page.bgColor
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 26
            anchors.rightMargin: 20
            anchors.topMargin: 22
            anchors.bottomMargin: 30
            spacing: 20

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 46
                spacing: 18

                Text {
                    text: "CV Library"
                    color: page.textColor
                    font.pixelSize: 31
                    font.bold: true
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                }

                CvSearchField {
                    Layout.preferredWidth: 485
                    Layout.preferredHeight: 46
                    textColor: page.textColor
                    mutedColor: page.mutedColor
                    lineColor: page.lineColor
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                CvLibraryBrowserPane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    cvs: page.cvs
                    selectedRow: page.selectedRow
                    panelColor: page.panelColor
                    lineColor: page.lineColor
                    textColor: page.textColor
                    mutedColor: page.mutedColor
                    blueColor: page.blueColor
                    greenColor: page.greenColor
                    yellowColor: page.yellowColor
                    purpleColor: page.purpleColor
                    onRowSelected: row => page.selectedRow = row
                }

                CvLibraryPreviewPanel {
                    Layout.preferredWidth: 405
                    Layout.fillHeight: true
                    selectedCv: page.selectedCv
                    panelColor: page.panelColor
                    lineColor: page.lineColor
                    textColor: page.textColor
                    mutedColor: page.mutedColor
                    blueColor: page.blueColor
                    greenColor: page.greenColor
                    yellowColor: page.yellowColor
                    purpleColor: page.purpleColor
                }
            }
        }
    }
}

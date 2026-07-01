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

                SearchField {
                    Layout.preferredWidth: 485
                    Layout.preferredHeight: 46
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 12

                    FilterBar {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 101
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 12

                        CategoriesPanel {
                            Layout.preferredWidth: 222
                            Layout.fillHeight: true
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 10

                            ListHeader {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 60
                            }

                            Repeater {
                                model: page.cvs

                                delegate: CvCard {
                                    required property var modelData
                                    required property int index

                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 132
                                    cv: modelData
                                    selected: index === page.selectedRow
                                    onClicked: page.selectedRow = index
                                }
                            }

                            Item {
                                Layout.fillHeight: true
                            }
                        }
                    }
                }

                PreviewPanel {
                    Layout.preferredWidth: 405
                    Layout.fillHeight: true
                }
            }
        }
    }

    component SearchField: TextField {
        id: field

        placeholderText: "Search CVs..."
        color: page.textColor
        placeholderTextColor: page.mutedColor
        leftPadding: 48
        rightPadding: 16
        font.pixelSize: 16
        selectionColor: "#14589a"
        selectedTextColor: "white"

        background: Rectangle {
            color: "#0a1823"
            border.color: page.lineColor
            border.width: 1
            radius: 8
        }

        Text {
            x: 16
            anchors.verticalCenter: parent.verticalCenter
            text: "⌕"
            color: "#c1cfdd"
            font.pixelSize: 28
        }
    }

    component FilterBar: Panel {
        color: page.panelColor
        border.color: page.lineColor
        radius: 8

        RowLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 22

            FilterBox {
                label: "Category"
                value: "All"
                Layout.preferredWidth: 185
            }
            FilterBox {
                label: "Language"
                value: "All"
                Layout.preferredWidth: 162
            }
            FilterBox {
                label: "Last Modified"
                value: "Any time"
                Layout.preferredWidth: 205
            }
            FilterBox {
                label: "Used in Jobs"
                value: "Any"
                Layout.preferredWidth: 155
            }

            IconButton {
                Layout.preferredWidth: 48
                Layout.preferredHeight: 52
                label: "☷"
            }

            Text {
                text: "Reset"
                color: page.blueColor
                font.pixelSize: 15
                Layout.leftMargin: 0
                Layout.alignment: Qt.AlignVCenter
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    component FilterBox: Rectangle {
        property string label: ""
        property string value: ""

        Layout.preferredHeight: 65
        radius: 7
        color: "#0f1f2a"
        border.color: "#2b3d4c"

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 12
            anchors.topMargin: 10
            anchors.bottomMargin: 9
            spacing: 3

            Text {
                text: label
                color: page.mutedColor
                font.pixelSize: 14
            }

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: value
                    color: page.textColor
                    font.pixelSize: 16
                    Layout.fillWidth: true
                }

                Text {
                    text: "⌄"
                    color: "#dbe7f2"
                    font.pixelSize: 20
                }
            }
        }
    }

    component IconButton: Rectangle {
        property string label: ""
        property bool active: false

        radius: 6
        color: active ? "#0f4c8f" : "#101f2a"
        border.color: active ? page.blueColor : "#2b3d4c"

        Text {
            anchors.centerIn: parent
            text: label
            color: active ? "white" : "#d7e5f2"
            font.pixelSize: 23
            font.bold: active
        }
    }

    component CategoriesPanel: Panel {
        color: page.panelColor
        border.color: page.lineColor
        radius: 8

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: "Categories"
                color: page.textColor
                font.pixelSize: 18
                font.bold: true
                Layout.leftMargin: 10
                Layout.topMargin: 12
                Layout.bottomMargin: 6
            }

            Repeater {
                model: [
                    ["All CVs", "7"],
                    ["C++ Developer", "2"],
                    ["Qt/QML Developer", "2"],
                    ["Flutter Developer", "1"],
                    ["1C Developer", "1"],
                    ["Office Job", "1"],
                    ["English CV", "5"],
                    ["Russian CV", "2"]
                ]

                delegate: CategoryRow {
                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    title: modelData[0]
                    count: modelData[1]
                    selected: index === 0
                }
            }

            Item {
                Layout.fillHeight: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 51
                radius: 6
                color: "#0f1f2b"
                border.color: "#2c4050"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 14
                    spacing: 10

                    Text {
                        text: "⚙"
                        color: page.blueColor
                        font.pixelSize: 22
                    }

                    Text {
                        text: "Manage Categories"
                        color: "#45a3ff"
                        font.pixelSize: 15
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    component CategoryRow: Rectangle {
        property string title: ""
        property string count: ""
        property bool selected: false

        Layout.preferredHeight: 48
        radius: 6
        color: selected ? "#143a67" : "transparent"
        border.width: selected ? 1 : 0
        border.color: selected ? "#146ec5" : "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 13
            anchors.rightMargin: 12
            spacing: 6

            Text {
                text: title
                color: selected ? "#59adff" : page.textColor
                font.pixelSize: 15
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Rectangle {
                implicitWidth: Math.max(25, countText.implicitWidth + 12)
                implicitHeight: 25
                radius: 6
                color: "#122532"
                border.color: "#2b3d4c"

                Text {
                    id: countText
                    anchors.centerIn: parent
                    text: count
                    color: selected ? "#58adff" : page.mutedColor
                    font.pixelSize: 14
                }
            }
        }
    }

    component ListHeader: Panel {
        color: page.panelColor
        border.color: page.lineColor
        radius: 8

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 12
            spacing: 12

            Text {
                text: "7 CVs"
                color: page.mutedColor
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                text: "Sort by:"
                color: page.mutedColor
                font.pixelSize: 15
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                text: "Last Modified"
                color: page.textColor
                font.pixelSize: 15
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                text: "⌄"
                color: "#dbe7f2"
                font.pixelSize: 20
                Layout.rightMargin: 10
                Layout.alignment: Qt.AlignVCenter
            }

            Rectangle {
                Layout.preferredWidth: 128
                Layout.preferredHeight: 48
                radius: 6
                color: "#0d1b25"
                border.color: "#2b3d4c"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 0

                    IconButton {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 58
                        label: "▦"
                        active: true
                    }

                    IconButton {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 58
                        label: "☷"
                    }
                }
            }
        }
    }

    component CvCard: Panel {
        id: card

        property var cv
        property bool selected: false
        signal clicked()

        color: selected ? "#0e2434" : page.panelColor
        border.color: selected ? page.blueColor : page.lineColor
        radius: 7

        MouseArea {
            anchors.fill: parent
            onClicked: card.clicked()
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 18
            anchors.topMargin: 18
            anchors.bottomMargin: 18
            spacing: 22

            PdfIcon {
                Layout.preferredWidth: 64
                Layout.preferredHeight: 89
            }

            ColumnLayout {
                Layout.preferredWidth: 225
                Layout.fillHeight: true
                spacing: 6

                Text {
                    text: cv.file
                    color: page.textColor
                    font.pixelSize: 22
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: cv.title
                    color: page.mutedColor
                    font.pixelSize: 16
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.topMargin: 4
                    spacing: 10

                    StatusChip {
                        label: cv.category
                        accent: page.categoryColor(cv.category)
                    }

                    StatusChip {
                        label: cv.language
                        accent: page.greenColor
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 76
                color: "#2f414d"
            }

            ColumnLayout {
                Layout.preferredWidth: 190
                Layout.fillHeight: true
                spacing: 18

                RowLayout {
                    spacing: 13
                    Text {
                        text: "▣"
                        color: "#c7d2df"
                        font.pixelSize: 19
                    }
                    Text {
                        text: "Updated " + cv.updated
                        color: page.mutedColor
                        font.pixelSize: 15
                    }
                }

                RowLayout {
                    spacing: 13
                    Text {
                        text: "▣"
                        color: "#c7d2df"
                        font.pixelSize: 19
                    }
                    Text {
                        text: "Used in " + cv.jobs + " jobs"
                        color: page.mutedColor
                        font.pixelSize: 15
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.preferredWidth: 104
                Layout.preferredHeight: 35
                radius: 5
                color: "transparent"
                border.color: page.blueColor

                Text {
                    anchors.centerIn: parent
                    text: "Open Details"
                    color: "#3ba0ff"
                    font.pixelSize: 14
                }
            }
        }
    }

    component PdfIcon: Item {
        Rectangle {
            anchors.fill: parent
            radius: 5
            color: "#f3f4f3"
            border.color: "#cfd3d6"
        }

        Rectangle {
            width: parent.width * 0.38
            height: parent.height * 0.24
            anchors.right: parent.right
            anchors.top: parent.top
            color: "#dde1e4"
            border.color: "#cfd3d6"
        }

        Rectangle {
            anchors.left: parent.left
            anchors.leftMargin: -2
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * 0.2
            width: parent.width * 0.68
            height: 24
            color: "#d93625"
            radius: 2

            Text {
                anchors.centerIn: parent
                text: "PDF"
                color: "white"
                font.pixelSize: 14
                font.bold: true
            }
        }
    }

    component PreviewPanel: Panel {
        color: page.panelColor
        border.color: page.lineColor
        radius: 8

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 53
                Layout.leftMargin: 16
                Layout.rightMargin: 14

                Text {
                    text: "Preview"
                    color: page.textColor
                    font.pixelSize: 18
                    font.bold: true
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.preferredWidth: 113
                    Layout.preferredHeight: 38
                    radius: 6
                    color: "#0c1a24"
                    border.color: "#2b3d4c"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 3
                        spacing: 0

                        IconButton {
                            Layout.fillHeight: true
                            Layout.preferredWidth: 56
                            label: "▦"
                            active: true
                        }

                        IconButton {
                            Layout.fillHeight: true
                            Layout.preferredWidth: 48
                            label: "▣"
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: page.lineColor
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 16
                spacing: 13

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 152
                    spacing: 24

                    PdfIcon {
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 91
                        Layout.alignment: Qt.AlignTop
                        Layout.topMargin: 20
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        Layout.topMargin: 8
                        spacing: 7

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: page.selectedCv.file
                                color: page.textColor
                                font.pixelSize: 22
                                font.bold: true
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: "★"
                                color: page.yellowColor
                                font.pixelSize: 25
                            }

                            Text {
                                text: "⋮"
                                color: page.mutedColor
                                font.pixelSize: 25
                            }
                        }

                        Text {
                            text: page.selectedCv.title
                            color: page.mutedColor
                            font.pixelSize: 16
                        }

                        StatusChip {
                            label: page.selectedCv.category
                            accent: page.categoryColor(page.selectedCv.category)
                        }

                        StatusChip {
                            label: page.selectedCv.language
                            accent: page.greenColor
                        }
                    }
                }

                Text {
                    text: "Description"
                    color: page.textColor
                    font.pixelSize: 16
                }

                Text {
                    Layout.fillWidth: true
                    text: page.selectedCv.description
                    color: page.textColor
                    font.pixelSize: 15
                    lineHeight: 1.25
                    wrapMode: Text.WordWrap
                }

                InfoDivider {}
                InfoRow {
                    label: "Language"
                    value: page.selectedCv.language
                }
                InfoDivider {}
                InfoRow {
                    label: "Last Modified"
                    value: page.selectedCv.updated
                }
                InfoDivider {}
                InfoRow {
                    label: "File Size"
                    value: page.selectedCv.size
                }
                InfoDivider {}
                InfoRow {
                    label: "Used in Jobs"
                    value: page.selectedCv.jobs
                }
                InfoDivider {}

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 8

                    Text {
                        text: "Linked Applications (" + page.selectedCv.jobs + ")"
                        color: page.textColor
                        font.pixelSize: 18
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Text {
                        text: "View all"
                        color: "#45a3ff"
                        font.pixelSize: 15
                    }
                }

                LinkedApplicationCard {
                    title: "Senior Qt/QML Developer"
                    company: "TechSoft Solutions"
                    status: "Interview"
                    date: "May 10, 2026"
                    accent: "#23d064"
                }

                LinkedApplicationCard {
                    title: "Qt Developer (Desktop)"
                    company: "Innovatech Systems"
                    status: "Applied"
                    date: "May 2, 2026"
                    accent: page.blueColor
                }

                LinkedApplicationCard {
                    title: "Qt/QML Engineer"
                    company: "CodeVision Ltd."
                    status: "Screening"
                    date: "Apr 28, 2026"
                    accent: page.yellowColor
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    Layout.topMargin: 2
                    radius: 7
                    color: "#0d1b25"
                    border.color: page.lineColor

                    Text {
                        anchors.centerIn: parent
                        text: "View all " + page.selectedCv.jobs + " applications"
                        color: "#45a3ff"
                        font.pixelSize: 15
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }

    component InfoDivider: Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: "#223440"
    }

    component InfoRow: RowLayout {
        property string label: ""
        property string value: ""

        Layout.fillWidth: true
        Layout.preferredHeight: 20

        Text {
            text: label
            color: page.textColor
            font.pixelSize: 15
            Layout.preferredWidth: 132
        }

        Text {
            text: value
            color: page.textColor
            font.pixelSize: 15
            Layout.fillWidth: true
        }
    }

    component LinkedApplicationCard: Rectangle {
        property string title: ""
        property string company: ""
        property string status: ""
        property string date: ""
        property color accent: page.blueColor

        Layout.fillWidth: true
        Layout.preferredHeight: 73
        radius: 7
        color: "#0d1b25"
        border.color: page.lineColor

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 42
                Layout.preferredHeight: 42
                radius: 6
                color: "#103155"
                border.color: "#1b62a8"

                Text {
                    anchors.centerIn: parent
                    text: "▣"
                    color: page.blueColor
                    font.pixelSize: 21
                    font.bold: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Text {
                    text: title
                    color: page.textColor
                    font.pixelSize: 14
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: company
                    color: page.mutedColor
                    font.pixelSize: 14
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                Layout.preferredWidth: 95
                spacing: 3

                RowLayout {
                    spacing: 6

                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: accent
                    }

                    Text {
                        text: status
                        color: accent
                        font.pixelSize: 14
                    }
                }

                Text {
                    text: date
                    color: page.mutedColor
                    font.pixelSize: 14
                }
            }
        }
    }
}

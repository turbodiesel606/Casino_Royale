import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    property var cvModel
    property var categorySummary: []
    property string resultSummary: ""
    property int selectedRow: 0
    property color panelColor: "#0b1b27"
    property color lineColor: "#243746"
    property color textColor: "#eef3f8"
    property color mutedColor: "#a8b5c2"
    property color blueColor: "#1687ff"
    property color greenColor: "#59d34d"
    property color yellowColor: "#ffbd21"
    property color purpleColor: "#b36bff"
    property string categoryFilter: ""
    property string languageFilter: ""
    property string sortMode: "Last Modified"

    signal rowSelected(int row)
    signal categoryFilterRequested(string category)
    signal languageFilterRequested(string language)
    signal sortModeRequested(string mode)
    signal clearFiltersRequested()

    function categoryOptions() {
        var options = ["All"]
        for (var i = 0; i < root.categorySummary.length; ++i) {
            var title = root.categorySummary[i].title
            if (title !== "All CVs")
                options.push(title)
        }
        return options
    }

    ColumnLayout {
        anchors.fill: parent
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
                    model: root.cvModel

                    delegate: CvCard {
                        required property int index

                        Layout.fillWidth: true
                        Layout.preferredHeight: 132
                        selected: index === root.selectedRow
                        onClicked: root.rowSelected(index)
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }

    component FilterBar: Panel {
        color: root.panelColor
        border.color: root.lineColor
        radius: 8

        RowLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 22

            FilterCombo {
                label: "Category"
                value: root.categoryFilter.length > 0 ? root.categoryFilter : "All"
                options: root.categoryOptions()
                onValueRequested: value => root.categoryFilterRequested(value === "All" ? "" : value)
                Layout.preferredWidth: 185
            }
            FilterCombo {
                label: "Language"
                value: root.languageFilter.length > 0 ? root.languageFilter : "All"
                options: ["All", "English", "German", "Russian"]
                onValueRequested: value => root.languageFilterRequested(value === "All" ? "" : value)
                Layout.preferredWidth: 162
            }
            FilterCombo {
                label: "Sort"
                value: root.sortMode
                options: ["Last Modified", "File Name", "Linked Jobs"]
                onValueRequested: value => root.sortModeRequested(value)
                Layout.preferredWidth: 205
            }
            FilterCombo {
                label: "Used in Jobs"
                value: "Any"
                options: ["Any"]
                Layout.preferredWidth: 155
            }

            IconButton {
                Layout.preferredWidth: 48
                Layout.preferredHeight: 52
                label: "Reset"

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.clearFiltersRequested()
                }
            }

            Text {
                text: "Reset"
                color: root.blueColor
                font.pixelSize: 15
                Layout.leftMargin: 0
                Layout.alignment: Qt.AlignVCenter

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.clearFiltersRequested()
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    component FilterCombo: Rectangle {
        property string label: ""
        property string value: ""
        property var options: []
        signal valueRequested(string value)

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
                color: root.mutedColor
                font.pixelSize: 14
            }

            RowLayout {
                Layout.fillWidth: true

                ComboBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    model: options
                    currentIndex: Math.max(0, options.indexOf(value))
                    onActivated: valueRequested(currentText)

                    contentItem: Text {
                        text: parent.displayText
                        color: root.textColor
                        font.pixelSize: 16
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        color: "transparent"
                    }
                }
            }
        }
    }

    component IconButton: Rectangle {
        property string label: ""
        property bool active: false

        radius: 6
        color: active ? "#0f4c8f" : "#101f2a"
        border.color: active ? root.blueColor : "#2b3d4c"

        Text {
            anchors.centerIn: parent
            text: label
            color: active ? "white" : "#d7e5f2"
            font.pixelSize: label.length > 2 ? 11 : 23
            font.bold: active
        }
    }

    component CategoriesPanel: Panel {
        color: root.panelColor
        border.color: root.lineColor
        radius: 8

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: "Categories"
                color: root.textColor
                font.pixelSize: 18
                font.bold: true
                Layout.leftMargin: 10
                Layout.topMargin: 12
                Layout.bottomMargin: 6
            }

            Repeater {
                model: root.categorySummary

                delegate: CategoryRow {
                    required property var modelData

                    Layout.fillWidth: true
                    title: modelData.title
                    count: String(modelData.count)
                    selected: modelData.selected
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
                        text: "+"
                        color: root.blueColor
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
                color: selected ? "#59adff" : root.textColor
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
                    color: selected ? "#58adff" : root.mutedColor
                    font.pixelSize: 14
                }
            }
        }
    }

    component ListHeader: Panel {
        color: root.panelColor
        border.color: root.lineColor
        radius: 8

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 12
            spacing: 12

            Text {
                text: root.resultSummary
                color: root.mutedColor
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                text: "Sort by:"
                color: root.mutedColor
                font.pixelSize: 15
                Layout.alignment: Qt.AlignVCenter
            }

            ComboBox {
                Layout.preferredWidth: 146
                Layout.preferredHeight: 38
                model: ["Last Modified", "File Name", "Linked Jobs"]
                currentIndex: Math.max(0, model.indexOf(root.sortMode))
                onActivated: root.sortModeRequested(currentText)

                contentItem: Text {
                    text: parent.displayText
                    color: root.textColor
                    font.pixelSize: 15
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color: "#0d1b25"
                    border.color: "#2b3d4c"
                    radius: 6
                }
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
                        label: "Grid"
                        active: true
                    }

                    IconButton {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 58
                        label: "List"
                    }
                }
            }
        }
    }

    component CvCard: Panel {
        id: card

        required property string fileName
        required property string title
        required property string category
        required property string categoryAccent
        required property string language
        required property string languageAccent
        required property string lastModifiedLabel
        required property string linkedApplicationCountLabel
        property bool selected: false
        signal clicked()

        color: selected ? "#0e2434" : root.panelColor
        border.color: selected ? root.blueColor : root.lineColor
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
                    text: card.fileName
                    color: root.textColor
                    font.pixelSize: 22
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: card.title
                    color: root.mutedColor
                    font.pixelSize: 16
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.topMargin: 4
                    spacing: 10

                    StatusChip {
                        label: card.category
                        accent: card.categoryAccent
                    }

                    StatusChip {
                        label: card.language
                        accent: card.languageAccent
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
                        text: "Updated"
                        color: "#c7d2df"
                        font.pixelSize: 13
                    }
                    Text {
                        text: card.lastModifiedLabel
                        color: root.mutedColor
                        font.pixelSize: 15
                    }
                }

                RowLayout {
                    spacing: 13
                    Text {
                        text: "Used"
                        color: "#c7d2df"
                        font.pixelSize: 13
                    }
                    Text {
                        text: card.linkedApplicationCountLabel
                        color: root.mutedColor
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
                border.color: root.blueColor

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
}

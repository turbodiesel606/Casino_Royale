import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    property var cvs: []
    property int selectedRow: 0
    property color panelColor: '#0b1b27'
    property color lineColor: '#243746'
    property color textColor: '#eef3f8'
    property color mutedColor: '#a8b5c2'
    property color blueColor: '#1687ff'
    property color greenColor: '#59d34d'
    property color yellowColor: '#ffbd21'
    property color purpleColor: '#b36bff'

    signal rowSelected(int row)

    function categoryColor(category) {
        if (category === 'Office Job')
            return root.yellowColor
        if (category === 'English CV')
            return root.purpleColor
        return root.blueColor
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
                model: root.cvs

                delegate: CvCard {
                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    Layout.preferredHeight: 132
                    cv: modelData
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
                color: root.blueColor
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
                color: root.mutedColor
                font.pixelSize: 14
            }

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: value
                    color: root.textColor
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
        border.color: active ? root.blueColor : "#2b3d4c"

        Text {
            anchors.centerIn: parent
            text: label
            color: active ? "white" : "#d7e5f2"
            font.pixelSize: 23
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
                text: "7 CVs"
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

            Text {
                text: "Last Modified"
                color: root.textColor
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
                    text: cv.file
                    color: root.textColor
                    font.pixelSize: 22
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: cv.title
                    color: root.mutedColor
                    font.pixelSize: 16
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.topMargin: 4
                    spacing: 10

                    StatusChip {
                        label: cv.category
                        accent: root.categoryColor(cv.category)
                    }

                    StatusChip {
                        label: cv.language
                        accent: root.greenColor
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
                        color: root.mutedColor
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

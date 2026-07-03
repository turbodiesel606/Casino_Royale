import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Panel {
    id: root

    property color textColor: "#eef3f8"
    property color mutedColor: "#a8b5c2"
    property color panelLineColor: "#263845"
    property int tableRightSafeMargin: 92

    component HeaderText: Text {
        color: root.mutedColor
        opacity: 0.9
        font.pixelSize: 12
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    component BodyText: Text {
        color: root.textColor
        font.pixelSize: 14
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    component Divider: Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: root.panelLineColor
    }

    component SmallActionButton: Rectangle {
        property string label: ""

        Layout.preferredWidth: 33
        Layout.preferredHeight: 28
        radius: 5
        color: "#102433"
        border.color: "#2a4050"

        Text {
            anchors.centerIn: parent
            text: label
            color: root.textColor
            font.pixelSize: 14
        }
    }
    Layout.fillWidth: true
    Layout.preferredHeight: 323

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: root.tableRightSafeMargin
            Layout.topMargin: 15
            Layout.bottomMargin: 11

            Text {
                text: "Recent CVs"
                color: root.textColor
                font.pixelSize: 18
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "View all"
                color: "#2588ff"
                font.pixelSize: 14
            }
        }

        Divider { }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 37
            Layout.leftMargin: 16
            Layout.rightMargin: root.tableRightSafeMargin
            spacing: 12

            HeaderText {
                text: "Name"
                Layout.preferredWidth: 350
                Layout.maximumWidth: 350
            }
            HeaderText {
                text: "Category"
                Layout.preferredWidth: 210
                Layout.maximumWidth: 210
            }
            HeaderText {
                text: "Language"
                Layout.preferredWidth: 150
                Layout.maximumWidth: 150
            }
            HeaderText {
                text: "Last Modified"
                Layout.preferredWidth: 135
                Layout.maximumWidth: 135
            }
            HeaderText {
                text: "Used in Jobs"
                Layout.preferredWidth: 120
                Layout.maximumWidth: 120
            }
            HeaderText {
                text: "Actions"
                Layout.preferredWidth: 86
                Layout.maximumWidth: 86
            }
        }

        Divider { }

        Repeater {
            model: [
                ["CV_Qt_2026.pdf", "Qt/QML Developer", "English", "May 12, 2026", "8 jobs"],
                ["CV_Cpp_Developer.pdf", "C++ Developer", "English", "May 5, 2026", "5 jobs"],
                ["CV_Remote.pdf", "C++ Developer", "English CV", "Apr 28, 2026", "3 jobs"],
                ["CV_Office.pdf", "Office Job", "English", "Apr 18, 2026", "2 jobs"],
                ["CV_Generic.pdf", "General", "English", "Apr 15, 2026", "1 job"]
            ]

            delegate: ColumnLayout {
                required property var modelData

                Layout.fillWidth: true
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 47
                    Layout.leftMargin: 16
                    Layout.rightMargin: root.tableRightSafeMargin
                    spacing: 12

                    RowLayout {
                        Layout.preferredWidth: 350
                        Layout.maximumWidth: 350
                        spacing: 14

                        Rectangle {
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 31
                            radius: 2
                            color: "#f1f5f8"

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 14
                                color: "#e63937"
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 1
                                text: "PDF"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 8
                            }
                        }

                        BodyText {
                            text: modelData[0]
                            Layout.fillWidth: true
                        }
                    }

                    StatusChip {
                        label: modelData[1]
                        accent: modelData[1] === "Office Job" ? "#ffbd21" : modelData[1] === "General" ? "#7f8b98" : "#1687ff"
                        Layout.preferredWidth: 210
                        Layout.maximumWidth: 210
                    }

                    StatusChip {
                        label: modelData[2]
                        accent: modelData[2] === "English CV" ? "#c86cff" : "#65bf4c"
                        Layout.preferredWidth: 150
                        Layout.maximumWidth: 150
                    }

                    BodyText {
                        text: modelData[3]
                        color: root.mutedColor
                        Layout.preferredWidth: 135
                        Layout.maximumWidth: 135
                    }

                    BodyText {
                        text: modelData[4]
                        color: root.mutedColor
                        Layout.preferredWidth: 120
                        Layout.maximumWidth: 120
                    }

                    RowLayout {
                        Layout.preferredWidth: 86
                        Layout.maximumWidth: 86
                        spacing: 10

                        SmallActionButton { label: "◉" }
                        SmallActionButton { label: "⋮" }
                    }
                }

                Divider { }
            }
        }
    }
}

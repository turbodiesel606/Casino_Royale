import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

RowLayout {
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
    Layout.fillWidth: true
    spacing: 14

    Panel {
        Layout.fillWidth: true
        Layout.preferredWidth: 460
        Layout.preferredHeight: 294

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 13

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: "Application Funnel"
                    color: root.textColor
                    font.pixelSize: 18
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "All time  ˅"
                    color: root.mutedColor
                    font.pixelSize: 14
                }
            }

            Repeater {
                model: [
                    ["Total Jobs", 1.0, "128  (100%)", "#167aff"],
                    ["Applied", 0.74, "62  (48.4%)", "#2ecb68"],
                    ["Interviews", 0.26, "18  (14.1%)", "#ffbb1d"],
                    ["Rejected", 0.47, "38  (29.7%)", "#ff4b49"],
                    ["Active", 0.58, "44  (34.4%)", "#00b8d4"]
                ]

                delegate: RowLayout {
                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 12

                    Text {
                        text: modelData[0]
                        color: root.textColor
                        font.pixelSize: 15
                        Layout.preferredWidth: 100
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 27
                        color: "transparent"

                        Rectangle {
                            width: parent.width * modelData[1]
                            height: parent.height
                            color: modelData[3]
                        }
                    }

                    Text {
                        text: modelData[2]
                        color: root.textColor
                        font.pixelSize: 14
                        Layout.preferredWidth: 88
                    }
                }
            }
        }
    }

    Panel {
        Layout.fillWidth: true
        Layout.preferredWidth: 610
        Layout.preferredHeight: 294

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
                    text: "Recent Applications"
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
                    text: "Job Title"
                    Layout.preferredWidth: 205
                    Layout.maximumWidth: 205
                }
                HeaderText {
                    text: "Company"
                    Layout.preferredWidth: 165
                    Layout.maximumWidth: 165
                }
                HeaderText {
                    text: "Status"
                    Layout.preferredWidth: 108
                    Layout.maximumWidth: 108
                }
                HeaderText {
                    text: "Applied On"
                    Layout.preferredWidth: 120
                    Layout.maximumWidth: 120
                }
                HeaderText {
                    text: ""
                    Layout.preferredWidth: 18
                    Layout.maximumWidth: 18
                }
            }

            Divider { }

            Repeater {
                model: [
                    ["C++/Qt Developer", "KDAB", "Applied", "May 12, 2026"],
                    ["Qt/QML Engineer", "The Qt Company", "Interview", "May 9, 2026"],
                    ["C++/Qt Developer", "Basler AG", "Applied", "May 6, 2026"],
                    ["Qt/QML Engineer", "Siemens", "Interview", "May 2, 2026"],
                    ["C++ Developer", "Bosch", "Rejected", "Apr 29, 2026"]
                ]

                delegate: ColumnLayout {
                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 39
                        Layout.leftMargin: 16
                        Layout.rightMargin: root.tableRightSafeMargin
                        spacing: 12

                        BodyText {
                            text: modelData[0]
                            Layout.preferredWidth: 205
                            Layout.maximumWidth: 205
                        }
                        BodyText {
                            text: modelData[1]
                            color: root.mutedColor
                            Layout.preferredWidth: 165
                            Layout.maximumWidth: 165
                        }
                        StatusChip {
                            label: modelData[2]
                            accent: modelData[2] === "Interview" ? "#ffbd21" : modelData[2] === "Rejected" ? "#ff4b49" : "#2ecb68"
                            Layout.preferredWidth: 108
                            Layout.maximumWidth: 108
                        }
                        BodyText {
                            text: modelData[3]
                            color: root.mutedColor
                            Layout.preferredWidth: 120
                            Layout.maximumWidth: 120
                        }
                        BodyText {
                            text: "›"
                            color: root.mutedColor
                            font.pixelSize: 24
                            horizontalAlignment: Text.AlignHCenter
                            Layout.preferredWidth: 18
                            Layout.maximumWidth: 18
                        }
                    }

                    Divider { }
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

RowLayout {
    id: root

    property var funnelModel
    property var recentApplicationsModel
    property color textColor: "#eef3f8"
    property color mutedColor: "#a8b5c2"
    property color panelLineColor: "#263845"
    property int tableRightSafeMargin: 92

    Layout.fillWidth: true
    spacing: 14

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
                    text: "All time"
                    color: root.mutedColor
                    font.pixelSize: 14
                }
            }

            Repeater {
                model: root.funnelModel

                delegate: RowLayout {
                    required property string title
                    required property real ratio
                    required property string value
                    required property string accent

                    Layout.fillWidth: true
                    spacing: 12

                    Text {
                        text: title
                        color: root.textColor
                        font.pixelSize: 15
                        Layout.preferredWidth: 100
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 27
                        color: "transparent"

                        Rectangle {
                            width: parent.width * ratio
                            height: parent.height
                            color: accent
                        }
                    }

                    Text {
                        text: value
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
                model: root.recentApplicationsModel

                delegate: ColumnLayout {
                    required property string jobTitle
                    required property string companyName
                    required property string statusLabel
                    required property string statusAccent
                    required property string appliedDateLabel

                    Layout.fillWidth: true
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 39
                        Layout.leftMargin: 16
                        Layout.rightMargin: root.tableRightSafeMargin
                        spacing: 12

                        BodyText {
                            text: jobTitle
                            Layout.preferredWidth: 205
                            Layout.maximumWidth: 205
                        }
                        BodyText {
                            text: companyName
                            color: root.mutedColor
                            Layout.preferredWidth: 165
                            Layout.maximumWidth: 165
                        }
                        StatusChip {
                            label: statusLabel
                            accent: statusAccent
                            Layout.preferredWidth: 108
                            Layout.maximumWidth: 108
                        }
                        BodyText {
                            text: appliedDateLabel
                            color: root.mutedColor
                            Layout.preferredWidth: 120
                            Layout.maximumWidth: 120
                        }
                        BodyText {
                            text: ">"
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

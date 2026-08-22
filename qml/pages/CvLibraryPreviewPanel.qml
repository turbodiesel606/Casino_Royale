import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    property var selectedCv: ({})
    property var linkedApplicationsModel
    property color panelColor: "#0b1b27"
    property color lineColor: "#243746"
    property color textColor: "#eef3f8"
    property color mutedColor: "#a8b5c2"
    property color blueColor: "#1687ff"
    property color greenColor: "#59d34d"
    property color yellowColor: "#ffbd21"
    property color purpleColor: "#b36bff"

    signal favoriteToggled()
    signal openCvRequested()

    PreviewPanel {
        anchors.fill: parent
    }

    component PreviewPanel: Panel {
        color: root.panelColor
        border.color: root.lineColor
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
                    color: root.textColor
                    font.pixelSize: 18
                    font.bold: true
                    Layout.fillWidth: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: root.lineColor
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
                                text: root.selectedCv.fileName || ""
                                color: root.textColor
                                font.pixelSize: 22
                                font.bold: true
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Text {
                                id: favoriteIcon
                                text: root.selectedCv.isFavorite ? "*" : ""
                                enabled: String(root.selectedCv.id || "").length > 0
                                color: !enabled
                                    ? "#81909D"
                                    : favoriteMouseArea.pressed
                                    ? "#D99A00"
                                    : (favoriteMouseArea.containsMouse
                                        ? "#FFD45C"
                                        : "#FFBD21")
                                font.pixelSize: 25

                                MouseArea {
                                    id: favoriteMouseArea
                                    anchors.fill: parent
                                    enabled: favoriteIcon.enabled
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.favoriteToggled()
                                }
                            }

                            Text {
                                text: "..."
                                color: root.mutedColor
                                font.pixelSize: 20
                            }
                        }

                        Text {
                            text: root.selectedCv.title || ""
                            color: root.mutedColor
                            font.pixelSize: 16
                        }

                        StatusChip {
                            label: root.selectedCv.category || ""
                            accent: root.selectedCv.categoryAccent || root.blueColor
                        }

                        StatusChip {
                            label: root.selectedCv.language || ""
                            accent: root.selectedCv.languageAccent || root.greenColor
                        }
                    }
                }

                Text {
                    text: "Description"
                    color: root.textColor
                    font.pixelSize: 16
                }

                Text {
                    Layout.fillWidth: true
                    text: root.selectedCv.description || ""
                    color: root.textColor
                    font.pixelSize: 15
                    lineHeight: 1.25
                    wrapMode: Text.WordWrap
                }

                InfoDivider {}
                InfoRow {
                    label: "Language"
                    value: root.selectedCv.language || ""
                }
                InfoDivider {}
                InfoRow {
                    label: "Last Modified"
                    value: root.selectedCv.lastModifiedLabel || ""
                }
                InfoDivider {}
                InfoRow {
                    label: "File Size"
                    value: root.selectedCv.fileSizeLabel || ""
                }
                InfoDivider {}
                InfoRow {
                    label: "Used in Jobs"
                    value: root.selectedCv.linkedApplicationCountLabel || ""
                }
                InfoDivider {}

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 8

                    Text {
                        text: "Linked Applications (" + (root.selectedCv.linkedApplicationCount || 0) + ")"
                        color: root.textColor
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

                Repeater {
                    model: root.linkedApplicationsModel

                    delegate: LinkedApplicationCard {
                        Layout.fillWidth: true
                    }
                }

                Rectangle {
                    id: viewAllApplicationsButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    Layout.topMargin: 2
                    radius: 7
                    color: viewAllApplicationsArea.pressed
                        ? "#132F61"
                        : (viewAllApplicationsArea.containsMouse
                            ? "#102A58"
                            : "#0B1B27")
                    border.color: "#223542"

                    Text {
                        anchors.centerIn: parent
                        text: "View all " + (root.selectedCv.linkedApplicationCount || 0) + " applications"
                        color: "#EEF3F8"
                        font.pixelSize: 15
                    }

                    MouseArea {
                        id: viewAllApplicationsArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.openCvRequested()
                    }
                }

                Item {
                    Layout.fillHeight: true
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
            color: root.textColor
            font.pixelSize: 15
            Layout.preferredWidth: 132
        }

        Text {
            text: value
            color: root.textColor
            font.pixelSize: 15
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
    }

    component LinkedApplicationCard: Rectangle {
        required property string jobTitle
        required property string companyName
        required property string statusLabel
        required property string statusAccent
        required property string dateLabel

        Layout.preferredHeight: 73
        radius: 7
        color: "#0d1b25"
        border.color: root.lineColor

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
                    text: "Job"
                    color: root.blueColor
                    font.pixelSize: 12
                    font.bold: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Text {
                    text: jobTitle
                    color: root.textColor
                    font.pixelSize: 14
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: companyName
                    color: root.mutedColor
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
                        color: statusAccent
                    }

                    Text {
                        text: statusLabel
                        color: statusAccent
                        font.pixelSize: 14
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Text {
                    text: dateLabel
                    color: root.mutedColor
                    font.pixelSize: 14
                }
            }
        }
    }
}

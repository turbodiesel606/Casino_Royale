import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page

    signal cancelRequested()
    signal saveRequested()

    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property color fieldColor: "#071925"
    readonly property color lineColor: "#223542"
    readonly property color accentBlue: "#1479ee"

    component FieldLabel: Text {
        color: page.textColor
        font.pixelSize: 15
    }

    component FormField: TextField {
        color: page.textColor
        font.pixelSize: 16
        leftPadding: 14
        rightPadding: 14
        selectByMouse: true

        background: Rectangle {
            color: page.fieldColor
            border.color: page.lineColor
            radius: 6
        }
    }

    component FormArea: TextArea {
        color: page.textColor
        font.pixelSize: 15
        leftPadding: 14
        rightPadding: 14
        topPadding: 12
        bottomPadding: 12
        wrapMode: TextArea.Wrap
        selectByMouse: true

        background: Rectangle {
            color: page.fieldColor
            border.color: page.lineColor
            radius: 6
        }
    }

    component TagChip: Rectangle {
        required property string label

        width: tagText.implicitWidth + 26
        height: 26
        radius: 13
        color: "#123f73"
        border.color: "#155caa"

        Text {
            id: tagText
            anchors.centerIn: parent
            text: label + "  Г—"
            color: "#dbe9ff"
            font.pixelSize: 14
        }
    }

    Panel {
        id: formCard
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 22
        width: Math.min(parent.width - 44, 930)
        height: Math.min(parent.height - 44, 820)
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 11

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 42

                Text {
                    text: "Add new Job"
                    color: page.textColor
                    font.pixelSize: 24
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                }

                Item { Layout.fillWidth: true }

                Button {
                    Layout.preferredWidth: 104
                    Layout.preferredHeight: 42
                    text: "Save"
                    onClicked: page.saveRequested()

                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15
                    }

                    background: Rectangle {
                        color: page.accentBlue
                        radius: 6
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 22
                rowSpacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Job Title" }

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        placeholderText: "Job title"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Job URL" }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        FormField {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            placeholderText: "https://..."

                            background: Rectangle {
                                color: page.fieldColor
                                border.color: page.lineColor
                                radius: 6
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 40
                            radius: 6
                            color: "#132737"
                            border.color: page.lineColor

                            Text {
                                anchors.centerIn: parent
                                text: "в†—"
                                color: page.textColor
                                font.pixelSize: 20
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Company" }

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        placeholderText: "Company"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Work Format" }

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        placeholderText: "Work format"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "City" }

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        placeholderText: "City"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Salary" }

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        placeholderText: "Salary"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Status" }

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        placeholderText: "Status"
                        color: "#eef3f8"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Application Date" }

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        placeholderText: "Application date"
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 7

                FieldLabel { text: "CV used" }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 49
                    color: page.fieldColor
                    border.color: page.lineColor
                    radius: 6

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 22
                        anchors.rightMargin: 18
                        spacing: 14

                        Rectangle {
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 37
                            radius: 3
                            color: "#e8edf1"

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 13
                                color: "#2a80f2"
                            }

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 2
                                text: "PDF"
                                color: "white"
                                font.pixelSize: 8
                                font.bold: true
                            }
                        }

                        Text {
                            text: "No CV selected"
                            color: page.textColor
                            font.pixelSize: 16
                            Layout.fillWidth: true
                        }

                        Button {
                            Layout.preferredWidth: 116
                            Layout.preferredHeight: 36
                            text: "Change CV"

                            contentItem: Text {
                                text: parent.text
                                color: page.textColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 14
                            }

                            background: Rectangle {
                                color: "#0b1b27"
                                border.color: "#3a5060"
                                radius: 6
                            }
                        }

                        Text {
                            text: "Г—"
                            color: page.textColor
                            font.pixelSize: 26
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 22
                rowSpacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Description" }

                    FormArea {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 86
                        placeholderText: "Job description"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Requirements" }

                    FormArea {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 86
                        placeholderText: "Requirements"
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 7

                FieldLabel { text: "Tech Stack" }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 39
                    color: page.fieldColor
                    border.color: page.lineColor
                    radius: 6

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 13
                        anchors.rightMargin: 13
                        spacing: 10

                        Text {
                            text: "Add technology..."
                            color: page.mutedColor
                            font.pixelSize: 14
                            Layout.fillWidth: true
                        }

                        Text {
                            text: "вЊ„"
                            color: page.textColor
                            font.pixelSize: 18
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 7

                FieldLabel { text: "Notes" }

                FormArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 62
                    placeholderText: "Notes"
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true

                Button {
                    Layout.preferredWidth: 88
                    Layout.preferredHeight: 40
                    text: "Discard"
                    onClicked: page.cancelRequested()

                    contentItem: Text {
                        text: parent.text
                        color: page.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15
                    }

                    background: Rectangle {
                        color: "#0b1b27"
                        border.color: page.lineColor
                        radius: 6
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }
    }
}

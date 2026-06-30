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
            text: label + "  ×"
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
                        text: "C++/Qt Developer"
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
                            text: "https://example.com/jobs/cpp-qt-developer"

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
                                text: "↗"
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
                        text: "KDAB"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Work Format" }

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        text: "⌂   Remote                                           ⌄"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "City" }

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        text: "Prague, Czech Republic"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Salary" }

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        text: "$4,500"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Status" }

                    FormField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        text: "●  Applied                                           ⌄"
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
                        text: "▣   May 12, 2026                                   ⌄"
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
                            text: "CV_Qt_2026.pdf"
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
                            text: "×"
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
                        text: "We are looking for an experienced C++/Qt Developer to build cross-platform desktop applications used by millions of users."
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    FieldLabel { text: "Requirements" }

                    FormArea {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 86
                        text: "•  5+ years of C++ development experience\n•  Strong knowledge of Qt, Widgets, QML\n•  Experience with CMake and modern C++"
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

                        TagChip { label: "C++" }
                        TagChip { label: "Qt" }
                        TagChip { label: "QML" }
                        TagChip { label: "CMake" }

                        Text {
                            text: "Add technology..."
                            color: page.mutedColor
                            font.pixelSize: 14
                            Layout.fillWidth: true
                        }

                        Text {
                            text: "⌄"
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
                    text: "Applied via company website. Strong focus on Qt 6, QML, and cross-platform development."
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

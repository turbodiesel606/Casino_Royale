import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page
    z: 20

    property color textColor: "#eef3f8"
    property color mutedColor: "#a8b5c2"
    property color panelLineColor: "#223542"
    property color blueColor: "#1687ff"

    signal applicationsRequested()

    component FieldLabel: Text {
        color: "#eef3f8"
        font.pixelSize: 13
        font.weight: Font.Medium
    }

    component FormField: TextField {
        color: "#eef3f8"
        placeholderTextColor: "#7f93a5"
        font.pixelSize: 15
        leftPadding: 12
        rightPadding: 12
        verticalAlignment: TextInput.AlignVCenter
        background: Rectangle {
            color: "#081923"
            border.color: "#263a48"
            radius: 5
        }
    }

    component FormArea: TextArea {
        color: "#eef3f8"
        placeholderTextColor: "#7f93a5"
        font.pixelSize: 14
        padding: 12
        wrapMode: TextEdit.WordWrap
        background: Rectangle {
            color: "#081923"
            border.color: "#263a48"
            radius: 5
        }
    }

    component TagChip: Rectangle {
        required property string label

        width: tagText.implicitWidth + 30
        height: 24
        radius: 7
        color: "#0b3d72"

        Text {
            id: tagText
            anchors.centerIn: parent
            text: parent.label + "  ×"
            color: "#dcefff"
            font.pixelSize: 13
        }
    }


        Rectangle {
            anchors.fill: parent
            color: "#07131d"
        }

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        spacing: 8

                        Button {
                            Layout.preferredWidth: 165
                            Layout.preferredHeight: 36
                            text: "Job Applications"
                            onClicked: page.applicationsRequested()

                            contentItem: Text {
                                text: parent.text
                                color: page.textColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 14
                            }

                            background: Rectangle {
                                color: "#0b1b27"
                                border.color: page.panelLineColor
                                border.width: 1
                                radius: 5
                            }
                        }

                        Button {
                            Layout.preferredWidth: 165
                            Layout.preferredHeight: 36
                            text: "Job Description"

                            contentItem: Text {
                                text: parent.text
                                color: page.textColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                            }

                            background: Rectangle {
                                color: "#0b1b27"
                                border.color: page.blueColor
                                border.width: 1
                                radius: 5
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Panel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 700
                        clip: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 34

                                Text {
                                    text: "Job Description"
                                    color: page.textColor
                                    font.bold: true
                                    font.pixelSize: 19
                                }

                                Item { Layout.fillWidth: true }

                                Button {
                                    Layout.preferredWidth: 112
                                    Layout.preferredHeight: 34
                                    text: "Edit"

                                    contentItem: Text {
                                        text: parent.text
                                        color: "white"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                    }

                                    background: Rectangle {
                                        color: "#1479ee"
                                        radius: 5
                                    }
                                }
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: 18
                                rowSpacing: 7

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Job Title" }
                                    FormField {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        text: "C++/Qt Developer"
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Job URL" }
                                    FormField {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        text: "https://example.com/jobs/cpp-qt-developer"
                                        rightPadding: 42

                                        Rectangle {
                                            anchors.right: parent.right
                                            anchors.top: parent.top
                                            anchors.bottom: parent.bottom
                                            width: 42
                                            color: "#102b3d"
                                            radius: 5

                                            Text {
                                                anchors.centerIn: parent
                                                text: "↗"
                                                color: page.textColor
                                                font.pixelSize: 16
                                            }
                                        }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Company" }
                                    FormField {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        text: "KDAB"
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Work Format" }
                                    FormField {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        text: "⌂  Remote"
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "City" }
                                    FormField {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        text: "Prague, Czech Republic"
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Salary" }
                                    FormField {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        text: "$4,500"
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Status" }
                                    FormField {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        text: "●  Applied"
                                        color: page.textColor
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Application Date" }
                                    FormField {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        text: "▣  May 12, 2026"
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 5

                                FieldLabel { text: "CV used" }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 48
                                    radius: 5
                                    color: "#081923"
                                    border.color: "#263a48"

                                    Rectangle {
                                        x: 24
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 24
                                        height: 30
                                        radius: 4
                                        color: "#2588ff"

                                        Text {
                                            anchors.centerIn: parent
                                            text: "PDF"
                                            color: "white"
                                            font.pixelSize: 8
                                            font.bold: true
                                        }
                                    }

                                    Text {
                                        x: 74
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "CV_Qt_2026.pdf"
                                        color: page.textColor
                                        font.pixelSize: 15
                                    }

                                    Button {
                                        anchors.right: closeCv.left
                                        anchors.rightMargin: 18
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 112
                                        height: 34
                                        text: "Change CV"

                                        contentItem: Text {
                                            text: parent.text
                                            color: page.textColor
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            font.pixelSize: 13
                                        }

                                        background: Rectangle {
                                            color: "transparent"
                                            border.color: "#40576a"
                                            radius: 5
                                        }
                                    }

                                    Text {
                                        id: closeCv
                                        anchors.right: parent.right
                                        anchors.rightMargin: 20
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "×"
                                        color: page.textColor
                                        font.pixelSize: 20
                                    }
                                }
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: 18
                                rowSpacing: 6

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Description" }
                                    FormArea {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 86
                                        text: "We are looking for an experienced C++/Qt Developer to build cross-platform desktop applications used by millions of users."
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
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
                                spacing: 5

                                FieldLabel { text: "Tech Stack" }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 38
                                    radius: 5
                                    color: "#081923"
                                    border.color: "#263a48"

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        spacing: 10

                                        TagChip { label: "C++" }
                                        TagChip { label: "Qt" }
                                        TagChip { label: "QML" }
                                        TagChip { label: "CMake" }

                                        Text {
                                            Layout.fillWidth: true
                                            text: "Add technology..."
                                            color: page.mutedColor
                                            font.pixelSize: 13
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        Text {
                                            text: "⌄"
                                            color: page.mutedColor
                                            font.pixelSize: 18
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 5

                                FieldLabel { text: "Notes" }

                                FormArea {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 60
                                    text: "Applied via company website. Strong focus on Qt 6, QML, and cross-platform development."
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.alignment: Qt.AlignBottom

                                Button {
                                    Layout.preferredWidth: 86
                                    Layout.preferredHeight: 38
                                    text: "Discard"

                                    contentItem: Text {
                                        text: parent.text
                                        color: page.textColor
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.pixelSize: 13
                                    }

                                    background: Rectangle {
                                        color: "#0b1b27"
                                        border.color: page.panelLineColor
                                        radius: 5
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                Button {
                                    Layout.preferredWidth: 132
                                    Layout.preferredHeight: 38
                                    text: "Save Changes"

                                    contentItem: Text {
                                        text: parent.text
                                        color: "white"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.pixelSize: 13
                                    }

                                    background: Rectangle {
                                        color: "#1479ee"
                                        radius: 5
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Panel {
                Layout.preferredWidth: 430
                Layout.fillHeight: true
                radius: 0

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 12

                    Text {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        text: "Preview"
                        color: page.textColor
                        font.bold: true
                        font.pixelSize: 18
                        verticalAlignment: Text.AlignVCenter
                    }

                    Panel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 382

                        Item {
                            anchors.fill: parent
                            anchors.margins: 18

                            Rectangle {
                                width: 72
                                height: 72
                                radius: 7
                                color: "#146ce0"

                                Text {
                                    anchors.centerIn: parent
                                    text: "KDAB"
                                    color: "white"
                                    font.pixelSize: 16
                                    font.bold: true
                                }
                            }

                            Text {
                                x: 94
                                y: 11
                                width: parent.width - 104
                                text: "C++/Qt Developer"
                                color: page.textColor
                                font.pixelSize: 20
                                font.bold: true
                            }

                            Text {
                                x: 94
                                y: 43
                                width: parent.width - 104
                                text: "KDAB"
                                color: page.mutedColor
                                font.pixelSize: 15
                            }

                            Column {
                                x: 0
                                y: 102
                                width: parent.width
                                spacing: 16

                                Repeater {
                                    model: [
                                        ["♕", "CV used", "CV_Qt_2026.pdf", "link"],
                                        ["♧", "Status", "Applied", "status"],
                                        ["▣", "Applied", "May 12, 2026", "text"],
                                        ["〽", "Salary", "$4,500", "text"],
                                        ["▤", "Format", "Remote", "text"],
                                        ["◴", "Last saved", "Today 12:25", "text"]
                                    ]

                                    delegate: Item {
                                        required property var modelData

                                        width: parent.width
                                        height: 26

                                        Text {
                                            x: 0
                                            width: 28
                                            height: parent.height
                                            text: modelData[0]
                                            color: page.mutedColor
                                            font.pixelSize: 17
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        Text {
                                            x: 36
                                            width: 110
                                            height: parent.height
                                            text: modelData[1]
                                            color: page.mutedColor
                                            font.pixelSize: 15
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        StatusChip {
                                            x: 154
                                            width: 74
                                            height: 26
                                            anchors.verticalCenter: parent.verticalCenter
                                            visible: modelData[3] === "status"
                                            label: modelData[2]
                                            accent: "#38c86b"
                                        }

                                        Text {
                                            x: 154
                                            width: parent.width - 154
                                            height: parent.height
                                            visible: modelData[3] !== "status"
                                            text: modelData[2]
                                            color: modelData[3] === "link" ? page.blueColor : page.textColor
                                            font.pixelSize: 15
                                            verticalAlignment: Text.AlignVCenter
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Panel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 108
                        clip: true

                        Repeater {
                            model: [
                                ["▤", "Requirements"],
                                ["‹›", "Tech Stack"]
                            ]

                            delegate: Rectangle {
                                required property var modelData
                                required property int index

                                x: 0
                                y: index * 54
                                width: parent.width
                                height: 54
                                color: "transparent"

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    height: 1
                                    color: page.panelLineColor
                                    visible: index === 0
                                }

                                Text {
                                    x: 18
                                    width: 32
                                    height: parent.height
                                    text: modelData[0]
                                    color: page.mutedColor
                                    font.pixelSize: 18
                                    verticalAlignment: Text.AlignVCenter
                                }

                                Text {
                                    x: 56
                                    width: parent.width - 96
                                    height: parent.height
                                    text: modelData[1]
                                    color: page.mutedColor
                                    font.pixelSize: 15
                                    verticalAlignment: Text.AlignVCenter
                                }

                                Text {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 18
                                    height: parent.height
                                    text: "›"
                                    color: page.textColor
                                    font.pixelSize: 22
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }

                    Panel {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 132

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 10

                            Text {
                                text: "Contacts"
                                color: page.textColor
                                font.bold: true
                                font.pixelSize: 16
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 54
                                radius: 6
                                color: "#0b1b27"
                                border.color: page.panelLineColor

                                Rectangle {
                                    x: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 38
                                    height: 38
                                    radius: 19
                                    color: "#d6a07a"

                                    Text {
                                        anchors.centerIn: parent
                                        text: "AM"
                                        color: "white"
                                        font.pixelSize: 12
                                        font.bold: true
                                    }
                                }

                                Text {
                                    x: 62
                                    y: 10
                                    text: "Anna Müller"
                                    color: page.textColor
                                    font.pixelSize: 14
                                    font.bold: true
                                }

                                Text {
                                    x: 62
                                    y: 30
                                    text: "HR Manager"
                                    color: page.mutedColor
                                    font.pixelSize: 12
                                }

                                Rectangle {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 132
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 34
                                    height: 34
                                    radius: 5
                                    color: "#0b1b27"
                                    border.color: page.panelLineColor

                                    Text {
                                        anchors.centerIn: parent
                                        text: "✉"
                                        color: page.textColor
                                        font.pixelSize: 15
                                    }
                                }

                                Rectangle {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 92
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 34
                                    height: 34
                                    radius: 5
                                    color: "#0b1b27"
                                    border.color: page.panelLineColor

                                    Text {
                                        anchors.centerIn: parent
                                        text: "in"
                                        color: page.blueColor
                                        font.pixelSize: 14
                                        font.bold: true
                                    }
                                }

                                Rectangle {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 74
                                    height: 34
                                    radius: 5
                                    color: "#0b1b27"
                                    border.color: page.panelLineColor

                                    Text {
                                        anchors.centerIn: parent
                                        text: "Open Link"
                                        color: page.textColor
                                        font.pixelSize: 12
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 54
                        spacing: 12

                        Button {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 54
                            text: "▣  Mark Next Step / Add Reminder"

                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 15
                            }

                            background: Rectangle {
                                color: "#1479ee"
                                border.color: "#1687ff"
                                radius: 6
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 58
                            Layout.preferredHeight: 54
                            radius: 6
                            color: "#0b1b27"
                            border.color: page.panelLineColor

                            Text {
                                anchors.centerIn: parent
                                text: "..."
                                color: page.textColor
                                font.pixelSize: 18
                            }
                        }
                    }
                }
            }
        }
}
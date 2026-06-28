import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page
    signal addJobRequested()

    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property color panelLineColor: "#263845"
    readonly property int pageMargin: 34
    readonly property int scrollbarWidth: 11
    readonly property int scrollbarGap: 12
    readonly property int tableRightSafeMargin: 92

    component HeaderText: Text {
        color: page.mutedColor
        opacity: 0.9
        font.pixelSize: 12
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    component BodyText: Text {
        color: page.textColor
        font.pixelSize: 14
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    component Divider: Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: page.panelLineColor
    }

    component IconBox: Rectangle {
        property string iconText: ""
        property color accentColor: "#1687ff"

        Layout.preferredWidth: 58
        Layout.preferredHeight: 58
        radius: 7
        color: "transparent"
        border.color: accentColor

        Text {
            anchors.centerIn: parent
            text: parent.iconText
            color: parent.accentColor
            font.pixelSize: 25
            font.bold: true
        }
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
            color: page.textColor
            font.pixelSize: 14
        }
    }

    Item {
        anchors.fill: parent
        anchors.leftMargin: page.pageMargin
        anchors.topMargin: page.pageMargin
        anchors.rightMargin: 16
        anchors.bottomMargin: page.pageMargin

        Flickable {
            id: flickable
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: scrollbarSeparator.left
            anchors.rightMargin: page.scrollbarGap
            clip: true
            contentWidth: width
            contentHeight: dashboard.implicitHeight
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                id: dashboard
                width: flickable.width
                spacing: 17

                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        Layout.preferredWidth: 450
                        Layout.preferredHeight: 42
                        placeholderText: "⌕    Search jobs, companies, contacts, CVs..."
                        color: page.textColor
                        placeholderTextColor: "#9aa8b7"
                        background: Rectangle {
                            radius: 7
                            color: "#0b1b27"
                            border.color: "#283b49"
                        }
                    }

                    Item { Layout.fillWidth: true }

                }

                Text {
                    text: "Dashboard"
                    color: page.textColor
                    font.bold: true
                    font.pixelSize: 30
                    Layout.topMargin: 6
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    Repeater {
                        model: [
                            { icon: "▣", title: "Total Jobs", value: "128", note: "▮  All time", color: "#1687ff" },
                            { icon: "➤", title: "Applied", value: "62", note: "▮  48.4% of total", color: "#2ecb68" },
                            { icon: "▣", title: "Interviews", value: "18", note: "▮  14.1% of total", color: "#ffbd21" },
                            { icon: "⊗", title: "Rejected", value: "38", note: "▮  29.7% of total", color: "#ff4b49" },
                            { icon: "♧", title: "Active", value: "44", note: "▮  In progress", color: "#00bfd5" }
                        ]

                        delegate: Panel {
                            required property var modelData

                            Layout.fillWidth: true
                            Layout.preferredHeight: 124

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 18
                                spacing: 18

                                IconBox {
                                    iconText: modelData.icon
                                    accentColor: modelData.color
                                }

                                ColumnLayout {
                                    spacing: 4

                                    Text {
                                        text: modelData.title
                                        color: page.textColor
                                        font.pixelSize: 15
                                    }

                                    Text {
                                        text: modelData.value
                                        color: page.textColor
                                        font.pixelSize: 30
                                        font.bold: true
                                    }

                                    Text {
                                        text: modelData.note
                                        color: modelData.color
                                        font.pixelSize: 14
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
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
                                    color: page.textColor
                                    font.pixelSize: 18
                                    font.bold: true
                                }

                                Item { Layout.fillWidth: true }

                                Text {
                                    text: "All time  ˅"
                                    color: page.mutedColor
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
                                        color: page.textColor
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
                                        color: page.textColor
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
                                Layout.rightMargin: page.tableRightSafeMargin
                                Layout.topMargin: 15
                                Layout.bottomMargin: 11

                                Text {
                                    text: "Recent Applications"
                                    color: page.textColor
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
                                Layout.rightMargin: page.tableRightSafeMargin
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
                                        Layout.rightMargin: page.tableRightSafeMargin
                                        spacing: 12

                                        BodyText {
                                            text: modelData[0]
                                            Layout.preferredWidth: 205
                                            Layout.maximumWidth: 205
                                        }
                                        BodyText {
                                            text: modelData[1]
                                            color: page.mutedColor
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
                                            color: page.mutedColor
                                            Layout.preferredWidth: 120
                                            Layout.maximumWidth: 120
                                        }
                                        BodyText {
                                            text: "›"
                                            color: page.mutedColor
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

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 323

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 0
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: page.tableRightSafeMargin
                            Layout.topMargin: 15
                            Layout.bottomMargin: 11

                            Text {
                                text: "Recent CVs"
                                color: page.textColor
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
                            Layout.rightMargin: page.tableRightSafeMargin
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
                                    Layout.rightMargin: page.tableRightSafeMargin
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
                                        color: page.mutedColor
                                        Layout.preferredWidth: 135
                                        Layout.maximumWidth: 135
                                    }

                                    BodyText {
                                        text: modelData[4]
                                        color: page.mutedColor
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
            }
        }

        Rectangle {
            id: scrollbarSeparator
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: verticalScrollBar.left
            anchors.rightMargin: page.scrollbarGap
            width: 1
            color: page.panelLineColor
            visible: flickable.contentHeight > flickable.height
        }

        Rectangle {
            id: verticalScrollBar
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: page.scrollbarWidth
            radius: page.scrollbarWidth / 2
            color: "#0a1823"
            border.color: "#223542"
            visible: flickable.contentHeight > flickable.height

            readonly property real scrollableHeight: Math.max(1, flickable.contentHeight - flickable.height)
            readonly property real thumbHeight: Math.min(height, Math.max(42, height * flickable.visibleArea.heightRatio))
            readonly property real thumbTravel: Math.max(0, height - thumbHeight)

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor

                onPressed: function(mouse) {
                    var targetRatio = (mouse.y - verticalScrollBar.thumbHeight / 2) / Math.max(1, verticalScrollBar.thumbTravel)
                    flickable.contentY = Math.max(0, Math.min(verticalScrollBar.scrollableHeight, targetRatio * verticalScrollBar.scrollableHeight))
                }
            }

            Rectangle {
                id: scrollThumb
                anchors.horizontalCenter: parent.horizontalCenter
                width: page.scrollbarWidth - 3
                height: verticalScrollBar.thumbHeight
                y: verticalScrollBar.thumbTravel * flickable.contentY / verticalScrollBar.scrollableHeight
                radius: width / 2
                color: "#a8b0b6"

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor

                    property real pressY: 0
                    property real pressContentY: 0

                    onPressed: function(mouse) {
                        var point = mapToItem(verticalScrollBar, mouse.x, mouse.y)
                        pressY = point.y
                        pressContentY = flickable.contentY
                    }

                    onPositionChanged: function(mouse) {
                        if (pressed) {
                            var point = mapToItem(verticalScrollBar, mouse.x, mouse.y)
                            var delta = point.y - pressY
                            var ratio = verticalScrollBar.scrollableHeight / Math.max(1, verticalScrollBar.thumbTravel)
                            flickable.contentY = Math.max(0, Math.min(verticalScrollBar.scrollableHeight, pressContentY + delta * ratio))
                        }
                    }
                }
            }
        }
    }
}

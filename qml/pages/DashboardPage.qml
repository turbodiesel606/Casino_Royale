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

                DashboardStatsRow {
                    textColor: page.textColor
                }

                DashboardActivityPanels {
                    textColor: page.textColor
                    mutedColor: page.mutedColor
                    panelLineColor: page.panelLineColor
                    tableRightSafeMargin: page.tableRightSafeMargin
                }

                DashboardRecentCvsPanel {
                    textColor: page.textColor
                    mutedColor: page.mutedColor
                    panelLineColor: page.panelLineColor
                    tableRightSafeMargin: page.tableRightSafeMargin
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

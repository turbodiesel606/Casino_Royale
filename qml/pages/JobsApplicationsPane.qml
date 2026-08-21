import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page

    property int selectedRow: 0
    property color textColor: "#eef3f8"
    property color mutedColor: "#a8b5c2"
    property color panelColor: "#0c1d29"
    property color panelLineColor: "#223542"
    property color blueColor: "#1687ff"
    property int tableHeaderHeight: 44
    property int tableRowHeight: 49
    property int tableFooterHeight: 68
    property var applicationsModel
    property int applicationCount: 0
    property var columns: []
    property string resultSummary: ""
    property string searchText: ""
    property string statusFilter: ""
    property var statusOptions: ["All Status", "Applied", "Interview", "Offer", "Test Task", "Rejected"]
    property var checkedApplicationIds: []
    property bool allVisibleApplicationsChecked: false
    property bool someVisibleApplicationsChecked: false

    signal rowSelected(int row)
    signal searchRequested(string text)
    signal statusFilterRequested(string status)
    signal rowCheckToggled(int row)
    signal allVisibleCheckedRequested(bool checked)

    readonly property real tableColumnLeft: 52
    readonly property real tableColumnRight: 18
    readonly property real tableColumnBaseEnd: 961
    readonly property real tableColumnScale: Math.max(
        0.0,
        (applicationsPanel.width - tableColumnLeft - tableColumnRight)
            / (tableColumnBaseEnd - tableColumnLeft))

    function scaledColumnX(column) {
        return tableColumnLeft
            + (column.x - tableColumnLeft) * tableColumnScale
    }

    function scaledColumnWidth(column) {
        return column.width * tableColumnScale
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    TextField {
                        Layout.preferredWidth: 360
                        Layout.preferredHeight: 45
                        leftPadding: 18
                        text: page.searchText
                        placeholderText: "⌕   Search applications..."
                        color: page.textColor
                        placeholderTextColor: page.mutedColor
                        font.pixelSize: 15

                        onTextChanged: {
                            if (text !== page.searchText)
                                page.searchRequested(text)
                        }

                        background: Rectangle {
                            color: "#0b1b27"
                            border.color: "#273b49"
                            radius: 7
                        }
                    }

                    ComboBox {
                        Layout.preferredWidth: 145
                        Layout.preferredHeight: 45
                        currentIndex: Math.max(0, page.statusOptions.indexOf(page.statusFilter.length > 0 ? page.statusFilter : "All Status"))
                        model: page.statusOptions
                        onActivated: page.statusFilterRequested(currentText === "All Status" ? "" : currentText)

                        contentItem: Text {
                            leftPadding: 14
                            text: parent.displayText
                            color: page.textColor
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 15
                        }

                        background: Rectangle {
                            color: "#0b1b27"
                            border.color: "#273b49"
                            radius: 7
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                Panel {
                    id: applicationsPanel
                    Layout.fillWidth: true
                    Layout.preferredHeight: page.tableHeaderHeight + page.applicationCount * page.tableRowHeight + page.tableFooterHeight
                    clip: true

                    Rectangle {
                        id: tableHeader
                        x: 0
                        y: 0
                        width: parent.width
                        height: page.tableHeaderHeight
                        color: "transparent"

                        SelectionCheckBox {
                            x: 12
                            anchors.verticalCenter: parent.verticalCenter
                            checkState: page.allVisibleApplicationsChecked
                                ? Qt.Checked
                                : (page.someVisibleApplicationsChecked ? Qt.PartiallyChecked : Qt.Unchecked)
                            nextCheckState: function() {
                                return page.allVisibleApplicationsChecked ? Qt.Unchecked : Qt.Checked
                            }
                            onClicked: page.allVisibleCheckedRequested(checkState === Qt.Checked)
                        }

                        Repeater {
                            model: page.columns

                            delegate: Text {
                                required property var modelData

                                x: page.scaledColumnX(modelData)
                                width: page.scaledColumnWidth(modelData)
                                height: parent.height
                                text: modelData.title
                                color: page.mutedColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Repeater {
                        model: page.applicationsModel

                        delegate: Rectangle {
                            required property int index
                            required property string companyName
                            required property string cvFileName
                            required property string dateLabel
                            required property string jobTitle
                            required property string statusLabel
                            required property string statusAccent
                            required property string appliedDate
                            required property string nextStep
                            required property string companyAccent
                            required property string companyInitials
                            required property var model

                            x: 0
                            y: page.tableHeaderHeight + index * page.tableRowHeight
                            width: applicationsPanel.width
                            height: page.tableRowHeight
                            color: index === page.selectedRow ? "#123757" : (rowMouse.containsMouse ? "#0d2738" : "transparent")
                            border.color: index === page.selectedRow ? page.blueColor : "transparent"
                            border.width: index === page.selectedRow ? 1 : 0

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 1
                                color: page.panelLineColor
                                visible: index !== page.selectedRow
                            }

                            MouseArea {
                                id: rowMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: page.rowSelected(index)
                            }

                            Rectangle {
                                x: page.scaledColumnX(page.columns[0])
                                width: 35
                                height: 35
                                anchors.verticalCenter: parent.verticalCenter
                                radius: 4
                                color: companyAccent

                                Text {
                                    anchors.centerIn: parent
                                    text: companyInitials
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: companyInitials.length > 3 ? 10 : 11
                                }
                            }

                            SelectionCheckBox {
                                x: 12
                                anchors.verticalCenter: parent.verticalCenter
                                z: 2
                                tristate: false
                                checked: page.checkedApplicationIds.indexOf(model.id) >= 0
                                onClicked: page.rowCheckToggled(index)
                            }

                            Text {
                                x: page.scaledColumnX(page.columns[0]) + 47
                                width: Math.max(0, page.scaledColumnWidth(page.columns[0]) - 47)
                                height: parent.height
                                text: companyName
                                color: page.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                x: page.scaledColumnX(page.columns[1])
                                width: page.scaledColumnWidth(page.columns[1])
                                height: parent.height
                                text: cvFileName
                                color: page.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                x: page.scaledColumnX(page.columns[2])
                                width: page.scaledColumnWidth(page.columns[2])
                                height: parent.height
                                text: dateLabel
                                color: page.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                x: page.scaledColumnX(page.columns[3])
                                width: page.scaledColumnWidth(page.columns[3])
                                height: parent.height
                                text: jobTitle
                                color: page.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            StatusChip {
                                x: page.scaledColumnX(page.columns[4])
                                width: Math.max(0, page.scaledColumnWidth(page.columns[4]) - 18)
                                height: 32
                                anchors.verticalCenter: parent.verticalCenter
                                label: statusLabel
                                accent: statusAccent
                            }

                            Text {
                                x: page.scaledColumnX(page.columns[5])
                                width: page.scaledColumnWidth(page.columns[5])
                                height: parent.height
                                text: appliedDate
                                color: page.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                x: page.scaledColumnX(page.columns[6])
                                width: page.scaledColumnWidth(page.columns[6])
                                height: parent.height
                                text: nextStep
                                color: page.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Rectangle {
                        id: tableFooter
                        x: 0
                            y: page.tableHeaderHeight + page.applicationCount * page.tableRowHeight
                        width: parent.width
                        height: page.tableFooterHeight
                        color: "transparent"

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            height: 1
                            color: page.panelLineColor
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 18
                            anchors.verticalCenter: parent.verticalCenter
                            text: page.resultSummary
                            color: page.mutedColor
                            font.pixelSize: 13
                        }

                        Row {
                            anchors.centerIn: parent
                            spacing: 10

                            Repeater {
                                model: ["‹", "1", "2", "3", "›"]

                                delegate: Rectangle {
                                    required property string modelData

                                    width: 34
                                    height: 34
                                    radius: 5
                                    color: modelData === "1" ? "#1479ee" : "#0b1b27"
                                    border.color: page.panelLineColor

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData
                                        color: page.textColor
                                        font.pixelSize: 15
                                    }
                                }
                            }
                        }

                        Rectangle {
                            anchors.right: parent.right
                            anchors.rightMargin: 14
                            anchors.verticalCenter: parent.verticalCenter
                            width: 126
                            height: 36
                            radius: 5
                            color: "#0b1b27"
                            border.color: page.panelLineColor

                            Text {
                                anchors.centerIn: parent
                                text: "10 per page  ⌄"
                                color: page.mutedColor
                                font.pixelSize: 14
                            }
                        }
                    }
                }
            }
        }

    }

    component SelectionCheckBox: CheckBox {
        id: selectionCheckBox
        implicitWidth: 24
        implicitHeight: 24
        tristate: true

        indicator: Rectangle {
            implicitWidth: 20
            implicitHeight: 20
            x: 2
            y: 2
            radius: 4
            color: selectionCheckBox.checkState === Qt.Unchecked ? "#0b1b27" : page.blueColor
            border.color: selectionCheckBox.checkState === Qt.Unchecked ? "#536674" : "#69a9ff"

            Text {
                anchors.centerIn: parent
                text: selectionCheckBox.checkState === Qt.PartiallyChecked ? "−" : "✓"
                visible: selectionCheckBox.checkState !== Qt.Unchecked
                color: "white"
                font.bold: true
                font.pixelSize: 13
            }
        }
    }

}

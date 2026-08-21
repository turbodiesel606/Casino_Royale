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
    property var previewDetails: []
    property string resultSummary: ""
    property string searchText: ""
    property string statusFilter: ""
    property var statusOptions: ["All Status", "Applied", "Interview", "Offer", "Test Task", "Rejected"]
    property string previewTitle: ""
    property string previewCompany: ""
    property string previewCompanyInitials: ""
    property string previewCompanyAccent: "#146ce0"
    property string previewNotes: ""
    property string previewStatusAccent: "#c2c7cb"
    property var checkedApplicationIds: []
    property bool allVisibleApplicationsChecked: false
    property bool someVisibleApplicationsChecked: false
    property bool deleteEnabled: false
    property bool deletionBusy: false

    signal rowSelected(int row)
    signal searchRequested(string text)
    signal statusFilterRequested(string status)
    signal applicationsRequested()
    signal descriptionRequested()
    signal rowCheckToggled(int row)
    signal allVisibleCheckedRequested(bool checked)
    signal deleteRequested()

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 22
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Button {
                        Layout.preferredWidth: 173
                        Layout.preferredHeight: 27
                        text: "Job Applications"
                        onClicked: page.applicationsRequested()

                        contentItem: Text {
                            text: parent.text
                            color: page.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 17
                            font.bold: true
                        }

                        background: Rectangle {
                            color: "#0b1b27"
                            border.color: page.blueColor
                            border.width: 1
                            radius: 4
                        }
                    }

                    Button {
                        Layout.preferredWidth: 173
                        Layout.preferredHeight: 27
                        text: "Job Description"
                        onClicked: page.descriptionRequested()

                        contentItem: Text {
                            text: parent.text
                            color: page.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 17
                            font.bold: true
                        }

                        background: Rectangle {
                            color: "#0b1b27"
                            border.color: page.blueColor
                            border.width: 1
                            radius: 4
                        }
                    }

                    Item { Layout.fillWidth: true }

                    DangerButton {
                        Layout.preferredWidth: 180
                        Layout.preferredHeight: 42
                        text: page.deletionBusy
                            ? "Deleting..."
                            : "Delete Selected (" + page.checkedApplicationIds.length + ")"
                        enabled: page.deleteEnabled
                        onClicked: page.deleteRequested()
                    }
                }

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

                                x: modelData.x
                                width: modelData.width
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
                                x: page.columns[0].x
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
                                x: page.columns[0].x + 47
                                width: page.columns[0].width - 47
                                height: parent.height
                                text: companyName
                                color: page.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                x: page.columns[1].x
                                width: page.columns[1].width
                                height: parent.height
                                text: cvFileName
                                color: page.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                x: page.columns[2].x
                                width: page.columns[2].width
                                height: parent.height
                                text: dateLabel
                                color: page.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                x: page.columns[3].x
                                width: page.columns[3].width
                                height: parent.height
                                text: jobTitle
                                color: page.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            StatusChip {
                                x: page.columns[4].x
                                width: page.columns[4].width - 18
                                height: 32
                                anchors.verticalCenter: parent.verticalCenter
                                label: statusLabel
                                accent: statusAccent
                            }

                            Text {
                                x: page.columns[5].x
                                width: page.columns[5].width
                                height: parent.height
                                text: appliedDate
                                color: page.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                x: page.columns[6].x
                                width: page.columns[6].width
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

        Panel {
            id: previewPanel
            Layout.preferredWidth: 382
            Layout.fillHeight: true
            radius: 0

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 35

                    Text {
                        text: "Preview"
                        color: page.textColor
                        font.bold: true
                        font.pixelSize: 17
                        verticalAlignment: Text.AlignVCenter
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        Layout.preferredWidth: 112
                        Layout.preferredHeight: 35
                        radius: 6
                        color: "#0b1b27"
                        border.color: page.panelLineColor

                        Rectangle {
                            x: 0
                            y: 0
                            width: parent.width / 2
                            height: parent.height
                            radius: 5
                            color: "#1479ee"

                            Text {
                                anchors.centerIn: parent
                                text: "▦"
                                color: "white"
                                font.pixelSize: 17
                            }
                        }

                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 18
                            anchors.verticalCenter: parent.verticalCenter
                            text: "▣"
                            color: page.textColor
                            font.pixelSize: 16
                        }
                    }
                }

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 490

                    Item {
                        anchors.fill: parent
                        anchors.margins: 14

                        Rectangle {
                            id: previewLogo
                            width: 62
                            height: 62
                            radius: 6
                            color: page.previewCompanyAccent

                            Text {
                                anchors.centerIn: parent
                                text: page.previewCompanyInitials
                                color: "white"
                                font.pixelSize: 15
                            }
                        }

                        Text {
                            x: 78
                            y: 8
                            width: parent.width - 84
                            text: page.previewTitle
                            color: page.textColor
                            font.pixelSize: 20
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Text {
                            x: 78
                            y: 36
                            width: parent.width - 84
                            text: page.previewCompany
                            color: page.mutedColor
                            font.pixelSize: 15
                            elide: Text.ElideRight
                        }

                        Column {
                            x: 0
                            y: 86
                            width: parent.width
                            spacing: 16

                            Repeater {
                                model: page.previewDetails

                                delegate: Item {
                                    required property var modelData

                                    width: parent.width
                                    height: 24

                                    Text {
                                        x: 0
                                        width: 26
                                        height: parent.height
                                        text: modelData[0]
                                        color: page.mutedColor
                                        font.pixelSize: 18
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Text {
                                        x: 32
                                        width: 104
                                        height: parent.height
                                        text: modelData[1]
                                        color: page.mutedColor
                                        font.pixelSize: 15
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    StatusChip {
                                        x: 140
                                        width: 84
                                        height: 28
                                        anchors.verticalCenter: parent.verticalCenter
                                        visible: modelData[1] === "Status"
                                        label: modelData[2]
                                        accent: page.previewStatusAccent
                                    }

                                    Text {
                                        x: 140
                                        width: parent.width - 140
                                        height: parent.height
                                        visible: modelData[1] !== "Status"
                                        text: modelData[2]
                                        color: modelData[1] === "CV used" ? "#2588ff" : page.textColor
                                        font.pixelSize: 15
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }

                        Text {
                            x: 0
                            y: 330
                            text: "Notes"
                            color: page.mutedColor
                            font.pixelSize: 16
                        }

                        Rectangle {
                            x: 0
                            y: 358
                            width: parent.width
                            height: 101
                            radius: 7
                            color: "#0b1b27"
                            border.color: "#283944"

                            Text {
                                anchors.fill: parent
                                anchors.margins: 13
                                text: page.previewNotes
                                color: page.textColor
                                font.pixelSize: 14
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 156
                    clip: true

                    Repeater {
                        model: [
                            ["▣", "Requirements"],
                            ["‹›", "Tech Stack"],
                            ["♙", "Contacts"]
                        ]

                        delegate: Rectangle {
                            required property var modelData
                            required property int index

                            x: 0
                            y: index * 52
                            width: parent.width
                            height: 52
                            color: "transparent"

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 1
                                color: page.panelLineColor
                                visible: index < 2
                            }

                            Text {
                                x: 16
                                width: 28
                                height: parent.height
                                text: modelData[0]
                                color: page.mutedColor
                                font.pixelSize: 18
                                verticalAlignment: Text.AlignVCenter
                            }

                            Text {
                                x: 52
                                width: parent.width - 88
                                height: parent.height
                                text: modelData[1]
                                color: page.mutedColor
                                font.pixelSize: 15
                                verticalAlignment: Text.AlignVCenter
                            }

                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 16
                                height: parent.height
                                text: "›"
                                color: page.textColor
                                font.pixelSize: 22
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    spacing: 10

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        text: "▣   Mark Next Step / Add Reminder"

                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 14
                        }

                        background: Rectangle {
                            color: "#1479ee"
                            radius: 6
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
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

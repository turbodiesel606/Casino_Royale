import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    property var cvModel
    property string resultSummary: ""
    property int selectedRow: 0
    property color panelColor: "#0b1b27"
    property color lineColor: "#243746"
    property color textColor: "#eef3f8"
    property color mutedColor: "#a8b5c2"
    property color blueColor: "#1687ff"
    property color greenColor: "#59d34d"
    property color yellowColor: "#ffbd21"
    property color purpleColor: "#b36bff"
    property string sortMode: "Last Modified"
    property bool archivedView: false
    property var checkedCvIds: []
    property int checkedLinkedCvCount: 0
    property int checkedUnlinkedCvCount: 0
    property bool allVisibleCvsChecked: false
    property bool someVisibleCvsChecked: false
    property bool mutationEnabled: false
    property bool mutationBusy: false
    property real selectionSizeMultiplier: 1.0

    signal rowSelected(int row)
    signal sortModeRequested(string mode)
    signal rowCheckToggled(int row)
    signal allVisibleCheckedRequested(bool checked)
    signal removeRequested()
    signal restoreRequested()
    signal permanentDeleteRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        ListHeader {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
        }

        Repeater {
            model: root.cvModel

            delegate: CvCard {
                required property int index
                required property var model

                Layout.fillWidth: true
                Layout.preferredHeight: 132
                selected: index === root.selectedRow
                checked: root.checkedCvIds.indexOf(model.id) >= 0
                onClicked: root.rowSelected(index)
                onCheckToggled: root.rowCheckToggled(index)
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    component ListHeader: Panel {
        color: root.panelColor
        border.color: root.lineColor
        radius: 8

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 12
            spacing: 12

            SelectionCheckBox {
                accentColor: root.blueColor
                sizeMultiplier: root.selectionSizeMultiplier
                checkState: root.allVisibleCvsChecked
                    ? Qt.Checked
                    : (root.someVisibleCvsChecked ? Qt.PartiallyChecked : Qt.Unchecked)
                nextCheckState: function() {
                    return root.allVisibleCvsChecked ? Qt.Unchecked : Qt.Checked
                }
                onClicked: root.allVisibleCheckedRequested(checkState === Qt.Checked)
            }

            Text {
                text: root.resultSummary
                color: root.mutedColor
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter
            }

            Item {
                Layout.fillWidth: true
            }

            PrimaryButton {
                visible: root.archivedView
                Layout.preferredWidth: 126
                Layout.preferredHeight: 40
                text: root.mutationBusy ? "Working..." : "Restore (" + root.checkedCvIds.length + ")"
                enabled: root.mutationEnabled
                onClicked: root.restoreRequested()
            }

            DangerButton {
                Layout.preferredWidth: root.archivedView ? 170 : 158
                Layout.preferredHeight: 40
                text: root.mutationBusy
                    ? "Working..."
                    : (root.archivedView
                        ? "Delete Permanently (" + root.checkedUnlinkedCvCount + ")"
                        : "Remove Selected (" + root.checkedCvIds.length + ")")
                enabled: root.mutationEnabled
                    && (!root.archivedView || root.checkedUnlinkedCvCount > 0)
                onClicked: root.archivedView
                    ? root.permanentDeleteRequested()
                    : root.removeRequested()

                ToolTip.visible: hovered && root.archivedView
                    && root.checkedCvIds.length > 0
                    && root.checkedUnlinkedCvCount === 0
                ToolTip.text: "Selected CVs are still used by job applications."
            }

            Text {
                text: "Sort by:"
                color: root.mutedColor
                font.pixelSize: 15
                Layout.alignment: Qt.AlignVCenter
            }

            ComboBox {
                Layout.preferredWidth: 146
                Layout.preferredHeight: 38
                model: ["Last Modified", "File Name", "Linked Jobs"]
                currentIndex: Math.max(0, model.indexOf(root.sortMode))
                onActivated: root.sortModeRequested(currentText)

                contentItem: Text {
                    text: parent.displayText
                    color: root.textColor
                    font.pixelSize: 15
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color: "#0d1b25"
                    border.color: "#2b3d4c"
                    radius: 6
                }
            }

        }
    }

    component CvCard: Panel {
        id: card

        required property string fileName
        required property string title
        required property string category
        required property string categoryAccent
        required property string language
        required property string languageAccent
        required property string lastModifiedLabel
        required property string linkedApplicationCountLabel
        property bool selected: false
        property bool checked: false
        signal clicked()
        signal checkToggled()

        color: selected ? "#0e2434" : root.panelColor
        border.color: root.lineColor
        radius: 7

        MouseArea {
            anchors.fill: parent
            onClicked: card.clicked()
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 18
            anchors.topMargin: 18
            anchors.bottomMargin: 18
            spacing: 22

            SelectionCheckBox {
                accentColor: root.blueColor
                sizeMultiplier: root.selectionSizeMultiplier
                tristate: false
                checked: card.checked
                Layout.alignment: Qt.AlignVCenter
                onClicked: card.checkToggled()
            }

            PdfIcon {
                Layout.preferredWidth: 64
                Layout.preferredHeight: 89
            }

            ColumnLayout {
                Layout.preferredWidth: 225
                Layout.fillHeight: true
                spacing: 6

                Text {
                    text: card.fileName
                    color: root.textColor
                    font.pixelSize: 22
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: card.title
                    color: root.mutedColor
                    font.pixelSize: 16
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.topMargin: 4
                    spacing: 10

                    StatusChip {
                        label: card.category
                        accent: card.categoryAccent
                    }

                    StatusChip {
                        label: card.language
                        accent: card.languageAccent
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 76
                color: "#2f414d"
            }

            ColumnLayout {
                Layout.preferredWidth: 190
                Layout.fillHeight: true
                spacing: 18

                RowLayout {
                    spacing: 13
                    Text {
                        text: "Updated"
                        color: "#c7d2df"
                        font.pixelSize: 13
                    }
                    Text {
                        text: card.lastModifiedLabel
                        color: root.mutedColor
                        font.pixelSize: 15
                    }
                }

                RowLayout {
                    spacing: 13
                    Text {
                        text: "Used"
                        color: "#c7d2df"
                        font.pixelSize: 13
                    }
                    Text {
                        text: card.linkedApplicationCountLabel
                        color: root.mutedColor
                        font.pixelSize: 15
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Rectangle {
                id: openDetailsButton
                Layout.preferredWidth: 104
                Layout.preferredHeight: 35
                radius: 5
                color: openDetailsArea.pressed
                    ? "#132F61"
                    : (openDetailsArea.containsMouse ? "#102A58" : "#0B1B27")
                border.color: "#223542"

                Text {
                    anchors.centerIn: parent
                    text: "Open Details"
                    color: "#EEF3F8"
                    font.pixelSize: 14
                }

                MouseArea {
                    id: openDetailsArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: card.clicked()
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
}

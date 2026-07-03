import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

RowLayout {
    id: root

    property color textColor: "#eef3f8"

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
                        color: root.textColor
                        font.pixelSize: 15
                    }

                    Text {
                        text: modelData.value
                        color: root.textColor
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

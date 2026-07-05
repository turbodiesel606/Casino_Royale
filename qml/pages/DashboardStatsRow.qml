import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

RowLayout {
    id: root

    property var statsModel
    property color textColor: "#eef3f8"

    Layout.fillWidth: true
    spacing: 14

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

    Repeater {
        model: root.statsModel

        delegate: Panel {
            required property string icon
            required property string title
            required property string value
            required property string note
            required property string accent

            Layout.fillWidth: true
            Layout.preferredHeight: 124

            RowLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 18

                IconBox {
                    iconText: icon
                    accentColor: accent
                }

                ColumnLayout {
                    spacing: 4

                    Text {
                        text: title
                        color: root.textColor
                        font.pixelSize: 15
                    }

                    Text {
                        text: value
                        color: root.textColor
                        font.pixelSize: 30
                        font.bold: true
                    }

                    Text {
                        text: note
                        color: accent
                        font.pixelSize: 14
                    }
                }
            }
        }
    }
}

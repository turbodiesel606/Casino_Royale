import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Page {
    id: root

    background: Item {}

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 24

        Text {
            text: "Settings"
            color: "#F4F8FF"
            font.pixelSize: 36
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 18

            Repeater {
                model: [
                    { label: "LLM Provider", value: "Not connected" },
                    { label: "Theme", value: "Cyberpunk Navy" },
                    { label: "Automation", value: "Manual mode" }
                ]

                NeonCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 180

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        Text {
                            text: modelData.label
                            color: "#9FB2D4"
                            font.pixelSize: 14
                        }

                        Text {
                            Layout.fillWidth: true
                            text: modelData.value
                            color: "#F4F8FF"
                            font.pixelSize: 22
                            font.bold: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}

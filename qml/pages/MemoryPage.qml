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
            text: "Memory"
            color: "#F4F8FF"
            font.pixelSize: 36
            font.bold: true
        }

        NeonCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 360
            glowColor: "#8B5CFF"

            ColumnLayout {
                anchors.fill: parent
                spacing: 18

                Text {
                    text: "Agent Memory"
                    color: "#F4F8FF"
                    font.pixelSize: 24
                    font.bold: true
                }

                Repeater {
                    model: [
                        "Prefers senior C++ and Qt roles",
                        "Highlights QML dashboards and desktop UI experience",
                        "Avoids roles requiring heavy backend ownership",
                        "Targets automation, AI tooling, and product engineering"
                    ]

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        radius: 12
                        color: "#0A244E"
                        border.width: 1
                        border.color: "#193E78"

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 16
                            anchors.right: parent.right
                            anchors.rightMargin: 16
                            text: modelData
                            color: "#D9E7FF"
                            font.pixelSize: 14
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}

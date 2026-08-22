import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: sidebar
    color: "#081723"
    border.color: "#203342"
    border.width: 1

    property int currentIndex: 0
    property bool actionsEnabled: true
    signal navigate(int index)
    signal addJob()
    signal addCv()

    readonly property var labels: ["Dashboard", "Job Applications", "CV Library", "Companies", "Contacts", "Settings"]
    readonly property var icons: ["▦", "▣", "▤", "♜", "♙", "⚙"]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.leftMargin: 10
            Layout.topMargin: 8
            Layout.bottomMargin: 18
            spacing: 10
            Rectangle {
                width: 42
 height: 42
 radius: 8
                gradient: Gradient {
                    GradientStop { position: 0.0
 color: "#2385ff" }
                    GradientStop { position: 1.0
 color: "#0d55ba" }
                }
                Text { anchors.centerIn: parent
 text: "JT"
 color: "white"
 font.bold: true
 font.pixelSize: 17 }
            }
            Text { text: "JobTracker"
 color: "#f0f4f8"
 font.bold: true
 font.pixelSize: 20 }
        }

        Repeater {
            model: sidebar.labels
            delegate: ItemDelegate {
                id: navigationButton
                required property int index
                required property string modelData
                Layout.fillWidth: true
                height: 51
                padding: 0
                hoverEnabled: true
                contentItem: RowLayout {
                    spacing: 15
                    Text { text: sidebar.icons[index]
 color: navigationButton.enabled ? (index === sidebar.currentIndex ? "#80baff" : "#eef3f8") : "#81909d"
 font.pixelSize: 25
 Layout.leftMargin: 16 }
                    Text { text: modelData
 color: navigationButton.enabled ? "#eef3f8" : "#81909d"
 font.pixelSize: 16 }
                }
                background: Rectangle {
                    radius: 7
                    color: !navigationButton.enabled
                        ? "#26343e"
                        : navigationButton.down
                            ? (index === sidebar.currentIndex ? "#0e2b5e" : "#132f61")
                            : navigationButton.hovered
                                ? (index === sidebar.currentIndex ? "#18498b" : "#102a58")
                                : (index === sidebar.currentIndex ? "#123c76" : "#0b1b27")
                    border.width: 1
                    border.color: !navigationButton.enabled
                        ? "#3b4a55"
                        : (index === sidebar.currentIndex ? "#1687ff" : "#223542")
                    Rectangle { visible: index === sidebar.currentIndex
 width: 3
 height: parent.height
 radius: 2
 color: "#1687ff" }
                }
                onClicked: {
                    if (index < 5)
                        sidebar.navigate(index)
                }
            }
        }

        Item { Layout.fillHeight: true }

        Rectangle { Layout.fillWidth: true
 height: 1
 color: "#21323f" }
        Text { text: "QUICK ACTIONS"
 color: "#93a1b0"
 font.pixelSize: 12
 Layout.topMargin: 16
 Layout.leftMargin: 7 }
        PrimaryButton {
            Layout.fillWidth: true
 Layout.preferredHeight: 48
            text: "+   Add Job"
            cornerRadius: 6
            labelPixelSize: 16
            labelFontWeight: Font.Normal
            enabled: sidebar.actionsEnabled
            onClicked: sidebar.addJob()
        }
        PrimaryButton {
            Layout.fillWidth: true
 Layout.preferredHeight: 43
 text: "+   Add CV"
            cornerRadius: 6
            labelPixelSize: 15
            labelFontWeight: Font.Normal
            enabled: sidebar.actionsEnabled
            onClicked: sidebar.addCv()
        }
    }
}

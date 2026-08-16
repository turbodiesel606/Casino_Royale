import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: sidebar
    color: "#081723"
    border.color: "#203342"
    border.width: 1

    property int currentIndex: 0
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
                required property int index
                required property string modelData
                Layout.fillWidth: true
                height: 51
                padding: 0
                contentItem: RowLayout {
                    spacing: 15
                    Text { text: sidebar.icons[index]
 color: index === sidebar.currentIndex ? "#80baff" : "#d1d9e1"
 font.pixelSize: 25
 Layout.leftMargin: 16 }
                    Text { text: modelData
 color: "#e3e8ee"
 font.pixelSize: 16 }
                }
                background: Rectangle {
                    radius: 7
                    color: index === sidebar.currentIndex ? "#103c6c" : (parent.hovered ? "#102331" : "transparent")
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
        Button {
            Layout.fillWidth: true
 Layout.preferredHeight: 48
            text: "+   Add Job"
            onClicked: sidebar.addJob()
            contentItem: Text { text: parent.text
 color: "#69a9ff"
 font.pixelSize: 16
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter }
            background: Rectangle { radius: 6
 color: "#0c2436"
 border.color: "#1575e6" }
        }
        Button {
            Layout.fillWidth: true
 Layout.preferredHeight: 43
 text: "+   Add CV"
            onClicked: sidebar.addCv()
            contentItem: Text { text: parent.text
 color: "#30cde1"
 font.pixelSize: 15
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter }
            background: Rectangle { radius: 6
 color: "#0c2436"
 border.color: "#008fa8" }
        }

        Rectangle {
            Layout.fillWidth: true
 Layout.preferredHeight: 76
 Layout.topMargin: 10
            radius: 7
 color: "#0b1c29"
 border.color: "#223442"
            RowLayout { anchors.fill: parent
 anchors.margins: 10
 spacing: 10
                Rectangle { width: 40
 height: 40
 radius: 20
 color: "#1266c9"
 Text { anchors.centerIn: parent
 text: "JD"
 color: "white"
 font.pixelSize: 16 } }
                ColumnLayout { spacing: 2
 Text { text: "John Doe"
 color: "#f0f4f8"
 font.pixelSize: 15 }
 Text { text: "john.doe@example.com"
 color: "#aab7c3"
 font.pixelSize: 11 } }
                Item { Layout.fillWidth: true }
 Text { text: "⌄"
 color: "#d5dde5"
 font.pixelSize: 19 }
            }
        }
    }
}

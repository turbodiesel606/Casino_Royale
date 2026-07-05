import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    default property alias content: contentArea.data
    property color cardColor: "#0B1B3D"
    property color borderColor: "#1E5D9D"
    property color glowColor: "#23E8FF"

    radius: 18
    color: cardColor
    border.width: 1
    border.color: borderColor

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.width: 1
        border.color: root.glowColor
        opacity: 0.18
    }

    Item {
        id: contentArea
        anchors.fill: parent
        anchors.margins: 24
    }
}

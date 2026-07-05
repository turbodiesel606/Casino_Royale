import QtQuick
import QtQuick.Controls

Button {
    id: control

    property bool active: false
    property color accentColor: "#23E8FF"
    property color secondaryAccentColor: "#8B5CFF"
    property color textColor: "#F4F8FF"
    property color mutedTextColor: "#9FB2D4"

    implicitHeight: 52
    padding: 0
    hoverEnabled: true

    contentItem: Text {
        text: control.text
        color: control.active || control.hovered ? control.textColor : control.mutedTextColor
        font.pixelSize: 15
        font.bold: control.active
        verticalAlignment: Text.AlignVCenter
        leftPadding: 22
        rightPadding: 18
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 14
        color: control.active
               ? "#123C76"
               : control.pressed
                 ? "#0E2B5E"
                 : control.hovered ? "#0A234C" : "transparent"
        border.width: control.active || control.hovered ? 1 : 0
        border.color: control.active ? control.accentColor : "#214A88"

        Rectangle {
            visible: control.active
            width: 4
            height: parent.height - 18
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            radius: 2
            color: control.accentColor
        }

        Rectangle {
            visible: control.active
            anchors.fill: parent
            anchors.margins: 1
            radius: parent.radius - 1
            color: control.secondaryAccentColor
            opacity: 0.12
        }
    }
}

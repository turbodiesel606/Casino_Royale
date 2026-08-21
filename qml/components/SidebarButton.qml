import QtQuick
import QtQuick.Templates as T

T.Button {
    id: control

    property bool active: false
    property color accentColor: "#1687FF"
    property color textColor: "#EEF3F8"
    property color mutedTextColor: "#EEF3F8"

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: 52
    padding: 0
    hoverEnabled: true

    contentItem: Text {
        text: control.text
        color: control.enabled
            ? (control.active || control.hovered
                ? control.textColor
                : control.mutedTextColor)
            : "#81909D"
        font.pixelSize: 15
        font.bold: control.active
        verticalAlignment: Text.AlignVCenter
        leftPadding: 22
        rightPadding: 18
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 14
        color: !control.enabled
            ? "#26343E"
            : control.down
                ? (control.active ? "#0E2B5E" : "#132F61")
                : control.hovered
                    ? (control.active ? "#18498B" : "#102A58")
                    : (control.active ? "#123C76" : "#0B1B27")
        border.width: control.visualFocus ? 2 : 1
        border.color: !control.enabled
            ? "#3B4A55"
            : (control.active ? control.accentColor : "#223542")

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

    }
}

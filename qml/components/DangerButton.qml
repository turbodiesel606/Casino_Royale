import QtQuick
import QtQuick.Controls

Button {
    id: control

    property color dangerColor: "#d94343"

    implicitHeight: 40
    leftPadding: 16
    rightPadding: 16

    contentItem: Text {
        text: control.text
        color: control.enabled ? "white" : "#81909d"
        font.pixelSize: 14
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 6
        color: !control.enabled
            ? "#26343e"
            : (control.down ? "#a92f2f" : (control.hovered ? "#eb5353" : control.dangerColor))
        border.color: control.enabled ? "#ff6b69" : "#3b4a55"
    }
}

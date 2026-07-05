import QtQuick
import QtQuick.Controls

Button {
    id: control

    property color accentColor: "#23E8FF"
    property color secondaryAccentColor: "#287CFF"
    property bool subtle: false

    implicitHeight: 46
    implicitWidth: 132
    padding: 0
    hoverEnabled: true

    contentItem: Text {
        text: control.text
        color: control.subtle ? "#D9E7FF" : "#031022"
        font.pixelSize: 15
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 14
        color: control.subtle
               ? (control.pressed ? "#132F61" : control.hovered ? "#102A58" : "#0A1D43")
               : (control.pressed ? control.secondaryAccentColor : control.hovered ? "#66F2FF" : control.accentColor)
        border.width: 1
        border.color: control.subtle ? "#2A5F9E" : "#80F7FF"
        opacity: control.enabled ? 1.0 : 0.45
    }
}

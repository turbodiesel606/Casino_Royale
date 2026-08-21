import QtQuick
import QtQuick.Templates as T

T.Button {
    id: control

    property color accentColor: "#1479EE"
    property color secondaryAccentColor: "#0F63C9"
    property bool subtle: false
    property bool selected: false
    property real cornerRadius: 6
    property int labelPixelSize: 15
    property int labelFontWeight: Font.DemiBold

    implicitWidth: Math.max(100, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: 40
    leftPadding: 16
    rightPadding: 16
    topPadding: 5
    bottomPadding: 5
    hoverEnabled: true

    contentItem: Text {
        text: control.text
        color: control.enabled ? "#EEF3F8" : "#81909D"
        font.pixelSize: control.labelPixelSize
        font.weight: control.labelFontWeight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: control.cornerRadius
        color: !control.enabled
            ? (control.selected || control.subtle ? "#26343E" : "#31506D")
            : control.down
                ? (control.selected
                    ? "#0E2B5E"
                    : (control.subtle ? "#132F61" : control.secondaryAccentColor))
                : control.hovered
                    ? (control.selected
                        ? "#18498B"
                        : (control.subtle ? "#102A58" : "#2588FF"))
                    : control.selected
                        ? "#123C76"
                        : (control.subtle ? "#0B1B27" : control.accentColor)
        border.width: control.visualFocus ? 2 : 1
        border.color: !control.enabled
            ? "#3B4A55"
            : (control.selected
                ? "#1687FF"
                : (control.subtle ? "#223542" : "#59ADFF"))
    }
}

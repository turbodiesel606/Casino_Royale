import QtQuick
import QtQuick.Templates as T

T.Button {
    id: control

    property color dangerColor: "#D94343"
    property real cornerRadius: 6
    property int labelPixelSize: 14
    property int labelFontWeight: Font.DemiBold

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
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
            ? "#26343E"
            : (control.down
                ? "#A92F2F"
                : (control.hovered ? "#EB5353" : control.dangerColor))
        border.width: control.visualFocus ? 2 : 1
        border.color: control.enabled ? "#FF6B69" : "#3B4A55"
    }
}

import QtQuick
import QtQuick.Shapes
import QtQuick.Templates as T

T.CheckBox {
    id: control

    property color accentColor: "#1687FF"
    property real sizeMultiplier: 1.0

    implicitWidth: 24 * sizeMultiplier
    implicitHeight: 24 * sizeMultiplier
    padding: 0
    spacing: 0
    tristate: true
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    contentItem: Item {
    }

    background: Item {
    }

    indicator: Rectangle {
        implicitWidth: 20 * control.sizeMultiplier
        implicitHeight: 20 * control.sizeMultiplier
        width: implicitWidth
        height: implicitHeight
        anchors.centerIn: parent
        radius: 4 * control.sizeMultiplier
        color: !control.enabled
            ? "#26343E"
            : control.down
                ? (control.checkState === Qt.Unchecked
                    ? "#132F61"
                    : (control.checkState === Qt.PartiallyChecked
                        ? "#0E2B5E"
                        : "#0F63C9"))
                : control.hovered
                    ? (control.checkState === Qt.Unchecked
                        ? "#102A58"
                        : (control.checkState === Qt.PartiallyChecked
                            ? "#18498B"
                            : "#2588FF"))
                    : control.checkState === Qt.Unchecked
                        ? "#0B1B27"
                        : (control.checkState === Qt.PartiallyChecked
                            ? "#123C76"
                            : control.accentColor)
        border.width: (control.visualFocus ? 2 : 1) * control.sizeMultiplier
        border.color: !control.enabled
            ? "#3B4A55"
            : control.visualFocus
                ? "#59ADFF"
                : control.checkState === Qt.Unchecked
                    ? "#536674"
                    : (control.checkState === Qt.PartiallyChecked
                        ? control.accentColor
                        : "#69A9FF")

        Shape {
            anchors.fill: parent
            visible: control.checkState === Qt.Checked
            antialiasing: true

            ShapePath {
                strokeColor: control.enabled ? "#FFFFFF" : "#81909D"
                strokeWidth: 2.4 * control.sizeMultiplier
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                startX: 5 * control.sizeMultiplier
                startY: 10.5 * control.sizeMultiplier

                PathLine {
                    x: 8.5 * control.sizeMultiplier
                    y: 14 * control.sizeMultiplier
                }

                PathLine {
                    x: 15 * control.sizeMultiplier
                    y: 6.5 * control.sizeMultiplier
                }
            }
        }

    }
}

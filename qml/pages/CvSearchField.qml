import QtQuick
import QtQuick.Controls

TextField {
    id: root

    property color textColor: '#eef3f8'
    property color mutedColor: '#a8b5c2'
    property color lineColor: '#243746'

    placeholderText: "Search CVs..."
    color: root.textColor
    placeholderTextColor: root.mutedColor
    leftPadding: 48
    rightPadding: 16
    font.pixelSize: 16
    selectionColor: "#14589a"
    selectedTextColor: "white"

    background: Rectangle {
        color: "#0a1823"
        border.color: root.lineColor
        border.width: 1
        radius: 8
    }

    Text {
        x: 16
        anchors.verticalCenter: parent.verticalCenter
        text: "⌕"
        color: "#c1cfdd"
        font.pixelSize: 28
    }
}

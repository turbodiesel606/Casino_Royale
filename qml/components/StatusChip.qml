import QtQuick

Rectangle {
    property string label: "Applied"
    property color accent: "#1687ff"
    implicitWidth: chipText.implicitWidth + 18
    implicitHeight: 27
    radius: 5
    color: Qt.rgba(accent.r, accent.g, accent.b, 0.18)
    border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.52)
    Text { id: chipText
 anchors.centerIn: parent
 text: parent.label
 color: parent.accent
 font.pixelSize: 14 }
}

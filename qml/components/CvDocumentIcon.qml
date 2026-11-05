import QtQuick

Item {
    Rectangle {
        anchors.fill: parent
        radius: 5
        color: "#f3f4f3"
        border.color: "#cfd3d6"
    }

    Rectangle {
        width: parent.width * 0.38
        height: parent.height * 0.24
        anchors.right: parent.right
        anchors.top: parent.top
        color: "#dde1e4"
        border.color: "#cfd3d6"
    }

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: -2
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height * 0.2
        width: parent.width * 0.68
        height: 24
        color: "#d93625"
        radius: 2

        Text {
            anchors.centerIn: parent
            text: "PDF"
            color: "white"
            font.pixelSize: 14
            font.bold: true
        }
    }
}

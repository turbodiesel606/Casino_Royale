import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var gate
    height: banner.visible ? banner.implicitHeight : 0

    Rectangle {
        id: banner
        anchors.left: parent.left
        anchors.right: parent.right
        implicitHeight: bannerContent.implicitHeight + 24
        height: implicitHeight
        visible: root.gate.retryNoticeVisible
        color: "#102330"
        border.color: "#59adff"

        RowLayout {
            id: bannerContent
            anchors.fill: parent
            anchors.margins: 12
            spacing: 16

            Text {
                Layout.fillWidth: true
                text: root.gate.retryNoticeMessage
                color: "#eef3f8"
                font.pixelSize: 15
                wrapMode: Text.Wrap
            }

            PrimaryButton {
                text: qsTr("Dismiss")
                subtle: true
                Layout.preferredWidth: 96
                Layout.preferredHeight: 36
                onClicked: root.gate.dismissRetryNotice()
            }
        }
    }

    Connections {
        target: root.gate

        function onOperationBlocked(category, reason) {
            blockedDialog.open()
            blockedDialog.forceActiveFocus()
        }

        function onStateChanged() {
            if (root.gate.shuttingDown)
                blockedDialog.close()
        }
    }

    Dialog {
        id: blockedDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(620, parent ? parent.width - 48 : 620)
        title: root.gate.blockedTitle
        modal: true
        focus: true
        closePolicy: Popup.NoAutoClose
        z: 1200

        contentItem: Text {
            text: root.gate.blockedMessage
            color: "#eef3f8"
            font.pixelSize: 15
            wrapMode: Text.Wrap
        }

        footer: DialogButtonBox {
            PrimaryButton {
                text: qsTr("OK")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: blockedDialog.accept()
            }
        }

        background: Rectangle {
            color: "#102330"
            border.color: "#ffbd21"
            border.width: 1
            radius: 8
        }
    }
}

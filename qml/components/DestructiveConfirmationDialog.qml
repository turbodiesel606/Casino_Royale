import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    property string message: ""
    property string confirmText: "Delete"
    signal confirmed()

    width: Math.min(590, parent ? parent.width - 48 : 590)
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose

    contentItem: Text {
        text: dialog.message
        color: "#eef3f8"
        font.pixelSize: 15
        wrapMode: Text.Wrap
    }

    footer: DialogButtonBox {
        PrimaryButton {
            text: "Cancel"
            subtle: true
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: dialog.reject()
        }

        DangerButton {
            text: dialog.confirmText
            DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
            onClicked: {
                dialog.confirmed()
                dialog.accept()
            }
        }
    }

    background: Rectangle {
        color: "#102330"
        border.color: "#ff6b69"
        border.width: 1
        radius: 8
    }
}

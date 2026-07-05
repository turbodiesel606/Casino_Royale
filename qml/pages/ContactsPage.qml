import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page

    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property var selectedContact: contactDirectoryController.selectedContact

    RowLayout {
        anchors.fill: parent
        spacing: 20

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 25
            Layout.topMargin: 30
            Layout.bottomMargin: 38

            ColumnLayout {
                anchors.fill: parent
                spacing: 16

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "Contacts"
                        color: page.textColor
                        font.pixelSize: 33
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    TextField {
                        Layout.preferredWidth: 285
                        Layout.preferredHeight: 43
                        text: contactDirectoryController.searchText
                        placeholderText: "Search contacts..."
                        color: page.textColor
                        placeholderTextColor: page.mutedColor
                        onTextChanged: {
                            if (text !== contactDirectoryController.searchText)
                                contactDirectoryController.setSearchText(text)
                        }
                        background: Rectangle {
                            color: "#0b1b27"
                            border.color: "#273b49"
                            radius: 7
                        }
                    }

                    Button {
                        text: "+  Add Contact"
                        Layout.preferredWidth: 170
                        Layout.preferredHeight: 43
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.pixelSize: 15
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: "#1479ee"
                            radius: 6
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    ComboBox {
                        Layout.preferredWidth: 145
                        Layout.preferredHeight: 43
                        model: ["All Companies", "KDAB", "TechSoft", "Vision Systems"]
                        currentIndex: Math.max(0, model.indexOf(contactDirectoryController.companyFilter.length > 0 ? contactDirectoryController.companyFilter : "All Companies"))
                        onActivated: contactDirectoryController.setCompanyFilter(currentText === "All Companies" ? "" : currentText)

                        contentItem: Text {
                            text: parent.displayText
                            color: page.textColor
                            font.pixelSize: 15
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 12
                            elide: Text.ElideRight
                        }

                        background: Rectangle {
                            color: "#0d1d28"
                            border.color: "#2a3c48"
                            radius: 6
                        }
                    }

                    ComboBox {
                        Layout.preferredWidth: 138
                        Layout.preferredHeight: 43
                        model: ["All Channels", "Email", "Telegram", "LinkedIn"]
                        currentIndex: Math.max(0, model.indexOf(contactDirectoryController.channelFilter.length > 0 ? contactDirectoryController.channelFilter : "All Channels"))
                        onActivated: contactDirectoryController.setChannelFilter(currentText === "All Channels" ? "" : currentText)

                        contentItem: Text {
                            text: parent.displayText
                            color: page.textColor
                            font.pixelSize: 15
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 12
                            elide: Text.ElideRight
                        }
                        background: Rectangle {
                            color: "#0d1d28"
                            border.color: "#2a3c48"
                            radius: 6
                        }
                    }

                    ComboBox {
                        Layout.preferredWidth: 130
                        Layout.preferredHeight: 43
                        model: ["Name", "Company", "Last Contact"]
                        currentIndex: Math.max(0, model.indexOf(contactDirectoryController.sortMode))
                        onActivated: contactDirectoryController.setSortMode(currentText)

                        contentItem: Text {
                            text: parent.displayText
                            color: page.textColor
                            font.pixelSize: 15
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 12
                            elide: Text.ElideRight
                        }
                        background: Rectangle {
                            color: "#0d1d28"
                            border.color: "#2a3c48"
                            radius: 6
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: contactDirectoryController.resultSummary
                        color: page.mutedColor
                        font.pixelSize: 15
                    }
                }

                Panel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            Layout.leftMargin: 12
                            spacing: 0

                            Repeater {
                                model: [["Name", 185], ["Role", 145], ["Company", 120], ["Related Job", 185], ["Email", 70], ["Telegram", 90], ["LinkedIn", 85], ["Last Contact", 105], ["", 25]]
                                delegate: Text {
                                    required property var modelData
                                    text: modelData[0]
                                    color: page.mutedColor
                                    font.pixelSize: 14
                                    Layout.preferredWidth: modelData[1]
                                }
                            }
                        }

                        Repeater {
                            model: contactDirectoryController.contactModel

                            delegate: ContactRow {
                                required property int index

                                Layout.fillWidth: true
                                selected: index === contactDirectoryController.selectedContactIndex
                                onClicked: contactDirectoryController.selectContact(index)
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 430
            Layout.fillHeight: true
            radius: 0

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 11

                Text {
                    text: "Preview"
                    color: page.textColor
                    font.pixelSize: 18
                    font.bold: true
                    Layout.leftMargin: 3
                }

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 198
                    color: "#0b1a25"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15

                        RowLayout {
                            Rectangle {
                                width: 65
                                height: 65
                                radius: 33
                                color: page.selectedContact.avatarAccent || "#8a98a5"

                                Text {
                                    anchors.centerIn: parent
                                    text: page.selectedContact.initials || ""
                                    color: "white"
                                    font.pixelSize: 21
                                    font.bold: true
                                }
                            }

                            ColumnLayout {
                                Text {
                                    text: page.selectedContact.displayName || ""
                                    color: page.textColor
                                    font.pixelSize: 21
                                    font.bold: true
                                }

                                Text {
                                    text: page.selectedContact.roleTitle || ""
                                    color: page.mutedColor
                                    font.pixelSize: 15
                                }

                                Text {
                                    text: page.selectedContact.companyName || ""
                                    color: page.mutedColor
                                    font.pixelSize: 15
                                }
                            }
                        }

                        ContactLine {
                            label: "Email"
                            value: page.selectedContact.email || ""
                        }
                        ContactLine {
                            label: "Telegram"
                            value: page.selectedContact.telegram || "-"
                        }
                        ContactLine {
                            label: "LinkedIn"
                            value: page.selectedContact.linkedin || "-"
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Repeater {
                        model: ["Contact Details", "Notes", "Interaction History"]
                        delegate: Text {
                            required property string modelData
                            required property int index
                            text: modelData
                            color: index === 0 ? "#2489ff" : page.mutedColor
                            font.pixelSize: 14
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 87
                    color: "#0b1a25"

                    Text {
                        anchors.fill: parent
                        anchors.margins: 13
                        text: "Notes\n" + (page.selectedContact.notes || "")
                        color: page.textColor
                        wrapMode: Text.WordWrap
                        font.pixelSize: 14
                    }
                }

                Text {
                    text: "Linked Company"
                    color: page.textColor
                    font.pixelSize: 16
                    font.bold: true
                }

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 87
                    color: "#0b1a25"

                    Text {
                        anchors.fill: parent
                        anchors.margins: 14
                        text: (page.selectedContact.companyName || "") + "\nView Company"
                        color: page.textColor
                        font.pixelSize: 15
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Text {
                    text: "Linked Application"
                    color: page.textColor
                    font.pixelSize: 16
                    font.bold: true
                }

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 87
                    color: "#0b1a25"

                    Text {
                        anchors.fill: parent
                        anchors.margins: 14
                        text: (page.selectedContact.relatedApplicationTitle || "") + "\nLast contact: " + (page.selectedContact.lastContactLabel || "")
                        color: page.textColor
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Text {
                    text: "Interaction History"
                    color: page.textColor
                    font.pixelSize: 16
                    font.bold: true
                }

                Repeater {
                    model: contactDirectoryController.interactionHistoryModel
                    delegate: InteractionRow {
                        Layout.fillWidth: true
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    component ContactRow: Rectangle {
        id: row

        required property string displayName
        required property string initials
        required property string avatarAccent
        required property string roleTitle
        required property string companyName
        required property string relatedApplicationTitle
        required property string email
        required property string telegram
        required property string linkedin
        required property string lastContactLabel
        property bool selected: false
        signal clicked()

        Layout.preferredHeight: 65
        color: selected ? "#123d6d" : (mouse.containsMouse ? "#102638" : "transparent")
        border.color: selected ? "#1687ff" : "#283a46"
        border.width: selected ? 1 : 0

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: row.clicked()
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            spacing: 0

            RowLayout {
                Layout.preferredWidth: 185
                spacing: 10

                Rectangle {
                    width: 38
                    height: 38
                    radius: 20
                    color: row.avatarAccent

                    Text {
                        anchors.centerIn: parent
                        text: row.initials
                        color: "white"
                        font.bold: true
                        font.pixelSize: 13
                    }
                }

                Text {
                    text: row.displayName
                    color: page.textColor
                    font.pixelSize: 15
                    font.bold: row.selected
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            Text {
                text: row.roleTitle
                color: page.textColor
                font.pixelSize: 14
                Layout.preferredWidth: 145
                elide: Text.ElideRight
            }

            Text {
                text: row.companyName
                color: page.textColor
                font.pixelSize: 14
                Layout.preferredWidth: 120
                elide: Text.ElideRight
            }

            Text {
                text: row.relatedApplicationTitle
                color: page.textColor
                font.pixelSize: 14
                Layout.preferredWidth: 185
                elide: Text.ElideRight
            }

            ChannelText {
                value: row.email
                activeText: "mail"
                Layout.preferredWidth: 70
            }

            ChannelText {
                value: row.telegram
                activeText: "tg"
                Layout.preferredWidth: 90
            }

            ChannelText {
                value: row.linkedin
                activeText: "in"
                Layout.preferredWidth: 85
            }

            Text {
                text: row.lastContactLabel
                color: page.mutedColor
                font.pixelSize: 14
                Layout.preferredWidth: 105
                elide: Text.ElideRight
            }

            Text {
                text: "..."
                color: page.mutedColor
                font.pixelSize: 20
                Layout.preferredWidth: 25
            }
        }
    }

    component ChannelText: Text {
        property string value: ""
        property string activeText: ""

        text: value.length > 0 ? activeText : "-"
        color: value.length > 0 ? "#298dff" : page.mutedColor
        font.pixelSize: 14
        font.bold: value.length > 0
        verticalAlignment: Text.AlignVCenter
    }

    component ContactLine: RowLayout {
        property string label: ""
        property string value: ""

        Layout.fillWidth: true
        Text {
            text: label
            color: "#288cff"
            font.pixelSize: 14
            Layout.preferredWidth: 74
        }
        Text {
            text: value
            color: page.textColor
            font.pixelSize: 14
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
    }

    component InteractionRow: RowLayout {
        required property string title
        required property string timestampLabel

        Layout.preferredHeight: 42
        spacing: 10

        Text {
            text: "o"
            color: "#2588ff"
            font.pixelSize: 20
        }

        ColumnLayout {
            Text {
                text: title
                color: page.textColor
                font.pixelSize: 15
                font.bold: true
            }

            Text {
                text: timestampLabel
                color: page.mutedColor
                font.pixelSize: 13
            }
        }
    }
}

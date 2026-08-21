import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page

    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property var selectedCompany: companyDirectoryController.selectedCompany

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 27
            Layout.topMargin: 25
            Layout.bottomMargin: 34

            ColumnLayout {
                anchors.fill: parent
                spacing: 16

                Text {
                    text: "Companies"
                    color: page.textColor
                    font.pixelSize: 33
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        Layout.preferredWidth: 335
                        Layout.preferredHeight: 43
                        text: companyDirectoryController.searchText
                        placeholderText: "Search companies..."
                        color: page.textColor
                        placeholderTextColor: page.mutedColor
                        onTextChanged: {
                            if (text !== companyDirectoryController.searchText)
                                companyDirectoryController.setSearchText(text)
                        }
                        background: Rectangle {
                            color: "#0b1b27"
                            border.color: "#273b49"
                            radius: 7
                        }
                    }

                    ComboBox {
                        Layout.preferredWidth: 122
                        Layout.preferredHeight: 43
                        model: ["Name", "Open Jobs", "Contacts"]
                        currentIndex: Math.max(0, model.indexOf(companyDirectoryController.sortMode))
                        onActivated: companyDirectoryController.setSortMode(currentText)

                        contentItem: Text {
                            text: parent.displayText
                            color: page.textColor
                            font.pixelSize: 14
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 12
                            elide: Text.ElideRight
                        }

                        background: Rectangle {
                            color: "#0e1e2a"
                            border.color: "#2c3e4a"
                            radius: 6
                        }
                    }

                    Item { Layout.fillWidth: true }

                    PrimaryButton {
                        text: "+  Add Company"
                        Layout.preferredHeight: 43
                        Layout.preferredWidth: 153
                        cornerRadius: 6
                        labelPixelSize: 15
                        labelFontWeight: Font.Normal
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
                            Layout.preferredHeight: 58
                            Layout.leftMargin: 18
                            spacing: 0

                            Repeater {
                                model: [["Company", 265], ["Website", 190], ["Open Jobs", 160], ["Contacts", 150], ["Last Activity", 130]]
                                delegate: Text {
                                    required property var modelData
                                    text: modelData[0]
                                    color: page.textColor
                                    font.pixelSize: 15
                                    Layout.preferredWidth: modelData[1]
                                }
                            }
                        }

                        Repeater {
                            model: companyDirectoryController.companyModel

                            delegate: CompanyRow {
                                required property int index

                                Layout.fillWidth: true
                                selected: index === companyDirectoryController.selectedCompanyIndex
                                onClicked: companyDirectoryController.selectCompany(index)
                            }
                        }

                        Item { Layout.fillHeight: true }

                        Text {
                            text: companyDirectoryController.resultSummary
                            color: page.mutedColor
                            font.pixelSize: 14
                            Layout.leftMargin: 18
                            Layout.bottomMargin: 18
                        }
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 455
            Layout.fillHeight: true
            radius: 0

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 33
                spacing: 14

                Text {
                    text: "Preview"
                    color: page.textColor
                    font.pixelSize: 19
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#2a3b46"
                }

                Text {
                    text: "Company Details"
                    color: page.textColor
                    font.pixelSize: 18
                    font.bold: true
                }

                RowLayout {
                    Rectangle {
                        width: 68
                        height: 68
                        radius: 6
                        color: page.selectedCompany.logoAccent || "#146ce0"

                        Text {
                            anchors.centerIn: parent
                            text: page.selectedCompany.logoText || ""
                            color: "white"
                            font.pixelSize: (page.selectedCompany.logoText || "").length > 2 ? 16 : 18
                            font.bold: true
                        }
                    }

                    ColumnLayout {
                        Text {
                            text: page.selectedCompany.name || ""
                            color: page.textColor
                            font.pixelSize: 24
                            font.bold: true
                        }

                        Text {
                            text: page.selectedCompany.website || ""
                            color: "#2588ff"
                            font.pixelSize: 15
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: "Description\n" + (page.selectedCompany.description || "")
                    color: page.textColor
                    font.pixelSize: 15
                    wrapMode: Text.WordWrap
                    lineHeight: 1.25
                }

                Text {
                    text: "Saved Notes"
                    color: page.textColor
                    font.pixelSize: 16
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 84
                    radius: 7
                    color: "#0c1b26"
                    border.color: "#2a3b46"

                    Text {
                        anchors.fill: parent
                        anchors.margins: 12
                        text: page.selectedCompany.notes || ""
                        color: page.mutedColor
                        wrapMode: Text.WordWrap
                        font.pixelSize: 14
                    }
                }

                Text {
                    text: "Linked Jobs   " + (page.selectedCompany.openJobCount || 0)
                    color: page.textColor
                    font.pixelSize: 16
                    font.bold: true
                    Layout.topMargin: 10
                }

                Repeater {
                            model: companyDirectoryController.linkedJobsModel
                            delegate: LinkedJobRow {
                                Layout.fillWidth: true
                            }
                }

                Text {
                    text: "Employees   " + (page.selectedCompany.contactCount || 0)
                    color: page.textColor
                    font.pixelSize: 16
                    font.bold: true
                    Layout.topMargin: 10
                }

                Repeater {
                            model: companyDirectoryController.linkedContactsModel
                            delegate: LinkedContactRow {
                                Layout.fillWidth: true
                            }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    component CompanyRow: Rectangle {
        id: row

        required property string name
        required property string website
        required property string logoText
        required property string logoAccent
        required property string openJobCountLabel
        required property string contactCountLabel
        required property string lastActivityLabel
        property bool selected: false
        signal clicked()

        Layout.preferredHeight: 88
        color: selected ? "#123757" : (mouse.containsMouse ? "#0e2738" : "transparent")
        border.color: selected ? "#1687ff" : "#263945"
        border.width: selected ? 1 : 0

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: row.clicked()
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 18
            spacing: 0

            RowLayout {
                Layout.preferredWidth: 265
                spacing: 14

                Rectangle {
                    width: 55
                    height: 55
                    radius: 6
                    color: row.logoAccent

                    Text {
                        anchors.centerIn: parent
                        text: row.logoText
                        color: "white"
                        font.pixelSize: row.logoText.length > 2 ? 16 : 19
                        font.bold: true
                    }
                }

                Text {
                    text: row.name
                    color: page.textColor
                    font.pixelSize: 17
                    font.bold: row.selected
                }
            }

            Text {
                text: row.website
                color: "#2b91ff"
                font.pixelSize: 15
                Layout.preferredWidth: 190
            }

            Text {
                text: row.openJobCountLabel
                color: "#2b91ff"
                font.pixelSize: 15
                Layout.preferredWidth: 160
            }

            Text {
                text: row.contactCountLabel
                color: "#2b91ff"
                font.pixelSize: 15
                Layout.preferredWidth: 150
            }

            Text {
                text: row.lastActivityLabel
                color: page.mutedColor
                font.pixelSize: 15
                Layout.preferredWidth: 130
            }
        }
    }

    component LinkedJobRow: Rectangle {
        required property string jobTitle
        required property string statusLabel
        required property string statusAccent

        Layout.preferredHeight: 64
        radius: 7
        color: "#0d1b25"
        border.color: "#2b3b45"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            Text {
                text: "Job"
                color: "#2588ff"
                font.pixelSize: 13
                font.bold: true
            }

            Text {
                text: jobTitle
                color: page.textColor
                font.pixelSize: 14
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            StatusChip {
                label: statusLabel
                accent: statusAccent
            }
        }
    }

    component LinkedContactRow: RowLayout {
        required property string displayName
        required property string roleTitle
        required property string linkedin
        required property string email

        Layout.preferredHeight: 38
        spacing: 8

        Text {
            text: displayName
            color: page.textColor
            font.pixelSize: 15
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Text {
            text: roleTitle
            color: page.mutedColor
            font.pixelSize: 14
            Layout.preferredWidth: 150
            elide: Text.ElideRight
        }

        Text {
            text: linkedin.length > 0 ? "in" : "-"
            color: linkedin.length > 0 ? "#2588ff" : page.mutedColor
            font.bold: true
            font.pixelSize: 15
        }

        Text {
            text: email.length > 0 ? "mail" : "-"
            color: email.length > 0 ? "#2588ff" : page.mutedColor
            font.pixelSize: 13
        }
    }
}

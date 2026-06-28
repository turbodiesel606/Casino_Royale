import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page
    signal addJobRequested()
    property int selectedRow: 0
    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property var applications: [
        ["KDAB", "CV_Qt_2026.pdf", "May 12, 2026", "C++/Qt Developer", "Applied", "May 12, 2026", "Test Task"],
        ["TechSoft", "CV_Qt_2026.pdf", "May 8, 2026", "Qt/QML Engineer", "Interview", "May 8, 2026", "Interview"],
        ["Vision Systems", "CV_Embedded.pdf", "May 5, 2026", "Embedded Developer", "Test Task", "May 5, 2026", "Test Task"],
        ["GreenWidget", "CV_General.pdf", "Apr 28, 2026", "Software Engineer", "Offer", "Apr 28, 2026", "Salary Discussion"],
        ["CodeCraft", "CV_Qt_2026.pdf", "Apr 22, 2026", "C++/Qt Developer", "Interview", "Apr 22, 2026", "Technical Interview"],
        ["Nexora", "CV_Backend.pdf", "Apr 18, 2026", "Backend Developer", "Applied", "Apr 18, 2026", "Screening Call"],
        ["ByteWorks", "CV_General.pdf", "Apr 10, 2026", "Software Engineer", "Rejected", "Apr 10, 2026", "—"],
        ["Innotech", "CV_Qt_2026.pdf", "Apr 2, 2026", "Qt/QML Engineer", "Test Task", "Apr 2, 2026", "Test Task"],
        ["Platforma", "CV_General.pdf", "Mar 28, 2026", "C++ Developer", "Interview", "Mar 28, 2026", "HR Interview"],
        ["DevSolutions", "CV_Embedded.pdf", "Mar 20, 2026", "Embedded C++ Engineer", "Applied", "Mar 20, 2026", "Screening Call"]
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0
        Item {
            Layout.fillWidth: true
 Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
 anchors.margins: 22
 spacing: 10
                RowLayout { Layout.fillWidth: true
                    Text { text: "Job Applications"
 color: page.textColor
 font.pixelSize: 34
 font.bold: true }
                    Item { Layout.fillWidth: true }
                    Button { text: "+   Add Job"
 Layout.preferredWidth: 122
 Layout.preferredHeight: 39
 onClicked: page.addJobRequested()
 contentItem: Text { text: parent.text
 color: "white"
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter
 font.pixelSize: 15 }
 background: Rectangle { radius: 5
 color: "#1479ee" } }
                }
                RowLayout { Layout.fillWidth: true
 spacing: 12
                    TextField { Layout.preferredWidth: 285
 Layout.preferredHeight: 45
 placeholderText: "⌕   Search applications..."
 color: page.textColor
 placeholderTextColor: page.mutedColor
 background: Rectangle { color: "#0b1b27"
 border.color: "#273b49"
 radius: 7 } }
                    ComboBox { Layout.preferredWidth: 145
 Layout.preferredHeight: 45
 model: ["⚱  All Status", "Applied", "Interview", "Offer"]
 contentItem: Text { leftPadding: 14
 text: parent.displayText
 color: page.textColor
 verticalAlignment: Text.AlignVCenter
 font.pixelSize: 15 }
 background: Rectangle { color: "#0b1b27"
 border.color: "#273b49"
 radius: 7 } }
                    Item { Layout.fillWidth: true }
                }
                Panel {
                    Layout.fillWidth: true
 Layout.fillHeight: true
                    ColumnLayout { anchors.fill: parent
 spacing: 0
                        Rectangle { Layout.fillWidth: true
 Layout.preferredHeight: 51
 color: "transparent"
                            RowLayout { anchors.fill: parent
 anchors.leftMargin: 17
 anchors.rightMargin: 17
 spacing: 12
                                Repeater { model: [["Company", 145], ["CV Used", 140], ["Date ↓", 120], ["Job Title", 180], ["Status", 108], ["Applied", 108], ["Next Step", 115]]
                                    delegate: Text { required property var modelData
 text: modelData[0]
 color: page.mutedColor
 font.pixelSize: 14
 Layout.preferredWidth: modelData[1] }
                                }
                            }
                        }
                        Repeater {
                            model: page.applications
                            delegate: Rectangle {
                                required property var modelData
                                required property int index
                                Layout.fillWidth: true
 Layout.preferredHeight: 58
                                color: index === page.selectedRow ? "#123757" : (rowMouse.containsMouse ? "#0d2738" : "transparent")
                                border.color: index === page.selectedRow ? "#1687ff" : "#263945"
 border.width: index === page.selectedRow ? 1 : 0
                                MouseArea { id: rowMouse
 anchors.fill: parent
 hoverEnabled: true
 onClicked: page.selectedRow = index }
                                RowLayout { anchors.fill: parent
 anchors.leftMargin: 17
 anchors.rightMargin: 17
 spacing: 12
                                    RowLayout { Layout.preferredWidth: 145
 spacing: 10
 Rectangle { width: 35
 height: 35
 radius: 4
 color: index === 0 ? "#146ce0" : "#153046"
 Text { anchors.centerIn: parent
 text: index === 0 ? "KDAB" : modelData[0].slice(0, 2)
 color: "white"
 font.bold: true
 font.pixelSize: 10 } }
 Text { text: modelData[0]
 color: page.textColor
 font.pixelSize: 14 } }
                                    Text { text: modelData[1]
 color: page.textColor
 font.pixelSize: 14
 Layout.preferredWidth: 140 }
                                    Text { text: modelData[2]
 color: page.textColor
 font.pixelSize: 14
 Layout.preferredWidth: 120 }
                                    Text { text: modelData[3]
 color: page.textColor
 font.pixelSize: 14
 Layout.preferredWidth: 180 }
                                    StatusChip { label: modelData[4]
 accent: modelData[4] === "Interview" ? "#ffbd21" : modelData[4] === "Offer" ? "#38c86b" : modelData[4] === "Rejected" ? "#ff4b49" : modelData[4] === "Test Task" ? "#16c5dd" : "#c2c7cb"
 Layout.preferredWidth: 108 }
                                    Text { text: modelData[5]
 color: page.textColor
 font.pixelSize: 14
 Layout.preferredWidth: 108 }
                                    Text { text: modelData[6]
 color: page.textColor
 font.pixelSize: 14
 Layout.preferredWidth: 115 }
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                        RowLayout { Layout.fillWidth: true
 Layout.preferredHeight: 69
 Layout.leftMargin: 18
 Layout.rightMargin: 18
                            Text { text: "Showing 1 to 10 of 24 applications"
 color: page.mutedColor
 font.pixelSize: 13 }
 Item { Layout.fillWidth: true }
 Text { text: "‹    1    2    3    ›"
 color: page.textColor
 font.pixelSize: 16 }
 Item { Layout.preferredWidth: 140 }
 Text { text: "10 per page  ⌄"
 color: page.mutedColor
 font.pixelSize: 14 }
                        }
                    }
                }
            }
        }
        Panel {
            Layout.preferredWidth: 382
 Layout.fillHeight: true
 radius: 0
            ColumnLayout { anchors.fill: parent
 anchors.margins: 16
 spacing: 13
                Text { text: "Preview"
 color: page.textColor
 font.bold: true
 font.pixelSize: 17 }
                Rectangle { Layout.fillWidth: true
 height: 1
 color: "#2a3a47" }
                RowLayout { Layout.fillWidth: true
 spacing: 16
 Rectangle { width: 62
 height: 62
 radius: 6
 color: "#146ce0"
 Text { anchors.centerIn: parent
 text: "KDAB"
 color: "white"
 font.pixelSize: 15 } }
 ColumnLayout { Text { text: page.applications[page.selectedRow][3]
 color: page.textColor
 font.pixelSize: 20
 font.bold: true }
 Text { text: page.applications[page.selectedRow][0]
 color: page.mutedColor
 font.pixelSize: 15 } } }
                Repeater { model: [["♧", "CV used", "CV_Qt_2026.pdf"], ["⌁", "Format", "Remote"], ["⌁", "Salary", "$4,500"], ["◷", "Status", "Applied"], ["▣", "Applied", "May 12, 2026"], ["⚑", "Next Step", "Test Task"]]
                    delegate: RowLayout { required property var modelData
 Layout.fillWidth: true
 Text { text: modelData[0]
 color: page.mutedColor
 font.pixelSize: 19
 Layout.preferredWidth: 26 }
 Text { text: modelData[1]
 color: page.mutedColor
 font.pixelSize: 15
 Layout.preferredWidth: 108 }
 Text { text: modelData[2]
 color: modelData[1] === "CV used" ? "#2588ff" : page.textColor
 font.pixelSize: 15 } }
                }
                Text { text: "Notes"
 color: page.mutedColor
 font.pixelSize: 16
 Layout.topMargin: 12 }
                Rectangle { Layout.fillWidth: true
 Layout.preferredHeight: 101
 radius: 7
 color: "#0b1b27"
 border.color: "#283944"
 Text { anchors.fill: parent
 anchors.margins: 13
 text: "Applied via company website.\n\nStrong focus on Qt 6, QML, and cross-platform development."
 color: page.textColor
 font.pixelSize: 14
 wrapMode: Text.WordWrap } }
                Repeater { model: ["▤   Requirements    ›", "‹›   Tech Stack    ›", "♙   Contacts    ›"]
 delegate: Rectangle { required property string modelData
 Layout.fillWidth: true
 Layout.preferredHeight: 52
 color: "#0c1b27"
 border.color: "#273944"
 Text { anchors.fill: parent
 anchors.margins: 15
 text: modelData
 color: page.mutedColor
 font.pixelSize: 15
 verticalAlignment: Text.AlignVCenter } } }
                Item { Layout.fillHeight: true }
                Button { Layout.fillWidth: true
 Layout.preferredHeight: 48
 text: "▣   Mark Next Step / Add Reminder"
 contentItem: Text { text: parent.text
 color: "white"
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter
 font.pixelSize: 14 }
 background: Rectangle { radius: 6
 color: "#1479ee" } }
            }
        }
    }
}

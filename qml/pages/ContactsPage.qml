import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page
    property int selectedRow: 0
    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property var contacts: [["Anna Müller", "HR Manager", "KDAB", "C++/Qt Developer", "2 days ago"], ["Thomas Becker", "Senior Software Engineer", "TechSoft", "Qt/QML Engineer", "1 week ago"], ["Julia Schneider", "Talent Acquisition", "KDAB", "C++/Qt Developer", "2 weeks ago"]]
    RowLayout { anchors.fill: parent
 spacing: 20
        Item { Layout.fillWidth: true
 Layout.fillHeight: true
 Layout.leftMargin: 25
 Layout.topMargin: 30
 Layout.bottomMargin: 38
            ColumnLayout { anchors.fill: parent
 spacing: 16
                RowLayout { Layout.fillWidth: true
 Text { text: "Contacts"
 color: page.textColor
 font.pixelSize: 33
 font.bold: true }
 Item { Layout.fillWidth: true }
 TextField { Layout.preferredWidth: 285
 Layout.preferredHeight: 43
 placeholderText: "⌕   Search contacts..."
 color: page.textColor
 placeholderTextColor: page.mutedColor
 background: Rectangle { color: "#0b1b27"
 border.color: "#273b49"
 radius: 7 } }
 Button { text: "♙  Add Contact    ⌄"
 Layout.preferredWidth: 170
 Layout.preferredHeight: 43
 contentItem: Text { text: parent.text
 color: "white"
 font.pixelSize: 15
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter }
 background: Rectangle { color: "#1479ee"
 radius: 6 } } }
                RowLayout { Layout.fillWidth: true
 Button { text: "♜  All Companies   ⌄"
 contentItem: Text { text: parent.text
 color: page.textColor
 font.pixelSize: 15
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter }
 background: Rectangle { color: "#0d1d28"
 border.color: "#2a3c48"
 radius: 6 } }
 Button { text: "⚱  All Channels   ⌄"
 contentItem: Text { text: parent.text
 color: page.textColor
 font.pixelSize: 15
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter }
 background: Rectangle { color: "#0d1d28"
 border.color: "#2a3c48"
 radius: 6 } }
 Item { Layout.fillWidth: true }
 Text { text: "3 contacts    ⟳   ⋯"
 color: page.mutedColor
 font.pixelSize: 15 } }
                Panel { Layout.fillWidth: true
 Layout.fillHeight: true
 ColumnLayout { anchors.fill: parent
 spacing: 0
                    RowLayout { Layout.fillWidth: true
 Layout.preferredHeight: 48
 Layout.leftMargin: 12
 spacing: 0
 Repeater { model: [["□", 36], ["Name  ↑", 157], ["Role", 120], ["Company", 96], ["Related Job", 148], ["Email", 62], ["Telegram", 82], ["LinkedIn", 85], ["Last Contact", 105], ["", 25]]
 delegate: Text { required property var modelData
 text: modelData[0]
 color: page.mutedColor
 font.pixelSize: 14
 Layout.preferredWidth: modelData[1] } } }
                    Repeater { model: page.contacts
 delegate: Rectangle { required property var modelData
 required property int index
 Layout.fillWidth: true
 Layout.preferredHeight: 65
 color: index === page.selectedRow ? "#123d6d" : (mouse.containsMouse ? "#102638" : "transparent")
 border.color: index === page.selectedRow ? "#1687ff" : "#283a46"
 border.width: index === page.selectedRow ? 1 : 0
 MouseArea { id: mouse
 anchors.fill: parent
 hoverEnabled: true
 onClicked: page.selectedRow = index }
 RowLayout { anchors.fill: parent
 anchors.leftMargin: 12
 spacing: 0
 Text { text: index === page.selectedRow ? "☑" : "□"
 color: "#54a3ff"
 font.pixelSize: 19
 Layout.preferredWidth: 36 }
 RowLayout { Layout.preferredWidth: 157
 spacing: 10
 Rectangle { width: 38
 height: 38
 radius: 20
 color: index === 0 ? "#dcc2ad" : "#8a98a5"
 Text { anchors.centerIn: parent
 text: modelData[0].slice(0, 1)
 color: "white"
 font.bold: true }
 Text { text: modelData[0]
 color: page.textColor
 font.pixelSize: 15
 font.bold: index === 0 } }
 Text { text: modelData[1]
 color: page.textColor
 font.pixelSize: 14
 Layout.preferredWidth: 120 }
 Text { text: modelData[2]
 color: page.textColor
 font.pixelSize: 14
 Layout.preferredWidth: 96 }
 Text { text: modelData[3]
 color: page.textColor
 font.pixelSize: 14
 Layout.preferredWidth: 148 }
 Text { text: "✉"
 color: "#298dff"
 font.pixelSize: 20
 Layout.preferredWidth: 62 }
 Text { text: index === 1 ? "–" : "➤"
 color: "#298dff"
 font.pixelSize: 19
 Layout.preferredWidth: 82 }
 Text { text: index === 2 ? "–" : "in"
 color: "#298dff"
 font.bold: true
 font.pixelSize: 19
 Layout.preferredWidth: 85 }
 Text { text: modelData[4]
 color: page.mutedColor
 font.pixelSize: 14
 Layout.preferredWidth: 105 }
 Text { text: "⋮"
 color: page.mutedColor
 font.pixelSize: 20
 Layout.preferredWidth: 25 } } } }
                    Item { Layout.fillHeight: true }
                } }
            }
        }
        Panel { Layout.preferredWidth: 430
 Layout.fillHeight: true
 radius: 0
 ColumnLayout { anchors.fill: parent
 anchors.margins: 12
 spacing: 11
            Text { text: "Preview"
 color: page.textColor
 font.pixelSize: 18
 font.bold: true
 Layout.leftMargin: 3 }
 Panel { Layout.fillWidth: true
 Layout.preferredHeight: 198
 color: "#0b1a25"
 ColumnLayout { anchors.fill: parent
 anchors.margins: 15
 RowLayout { Rectangle { width: 65
 height: 65
 radius: 33
 color: "#dbc3ae"
 Text { anchors.centerIn: parent
 text: "A"
 color: "white"
 font.pixelSize: 25 }
 ColumnLayout { Text { text: page.contacts[page.selectedRow][0]
 color: page.textColor
 font.pixelSize: 21
 font.bold: true }
 Text { text: page.contacts[page.selectedRow][1]
 color: page.mutedColor
 font.pixelSize: 15 }
 Text { text: page.contacts[page.selectedRow][2]
 color: page.mutedColor
 font.pixelSize: 15 } } }
 Repeater { model: [["✉", "anna.mueller@kdab.com"], ["➤", "@anna_mueller_kdab"], ["in", "linkedin.com/in/anna-mueller-kdab"]]
 delegate: RowLayout { required property var modelData
 Layout.fillWidth: true
 Text { text: modelData[0]
 color: "#288cff"
 font.pixelSize: 19
 Layout.preferredWidth: 34 }
 Text { text: modelData[1]
 color: page.textColor
 font.pixelSize: 15
 Layout.fillWidth: true }
 Text { text: "▣"
 color: page.mutedColor
 font.pixelSize: 15 } } } } }
            RowLayout { Layout.fillWidth: true
 Repeater { model: ["Contact Details", "Notes", "Interaction History"]
 delegate: Text { required property string modelData
 text: modelData
 color: index === 0 ? "#2489ff" : page.mutedColor
 font.pixelSize: 14
 Layout.fillWidth: true
 horizontalAlignment: Text.AlignHCenter } } }
            Panel { Layout.fillWidth: true
 Layout.preferredHeight: 87
 color: "#0b1a25"
 Text { anchors.fill: parent
 anchors.margins: 13
 text: "Notes\nVery positive screening call. Strong Qt and C++ background. Interested in long-term opportunities."
 color: page.textColor
 wrapMode: Text.WordWrap
 font.pixelSize: 14 } }
            Text { text: "Linked Company"
 color: page.textColor
 font.pixelSize: 16
 font.bold: true }
 Panel { Layout.fillWidth: true
 Layout.preferredHeight: 87
 color: "#0b1a25"
 Text { anchors.fill: parent
 anchors.margins: 14
 text: "KDAB\nSoftware Development                       View Company"
 color: page.textColor
 font.pixelSize: 15
 verticalAlignment: Text.AlignVCenter } }
            Text { text: "Linked Application"
 color: page.textColor
 font.pixelSize: 16
 font.bold: true }
 Panel { Layout.fillWidth: true
 Layout.preferredHeight: 87
 color: "#0b1a25"
 Text { anchors.fill: parent
 anchors.margins: 14
 text: "▣   C++/Qt Developer                         Applied\n     Applied on May 12, 2026 · Screening"
 color: page.textColor
 font.pixelSize: 14
 verticalAlignment: Text.AlignVCenter } }
            Text { text: "Interaction History"
 color: page.textColor
 font.pixelSize: 16
 font.bold: true }
 Repeater { model: [["●", "Screening call", "May 14, 2026 at 10:30"], ["●", "Follow up", "May 9, 2026 at 14:20"], ["●", "LinkedIn connection", "May 7, 2026 at 09:15"]]
 delegate: RowLayout { required property var modelData
 Layout.fillWidth: true
 Text { text: modelData[0]
 color: "#2588ff"
 font.pixelSize: 25 }
 ColumnLayout { Text { text: modelData[1]
 color: page.textColor
 font.pixelSize: 15
 font.bold: true }
 Text { text: modelData[2]
 color: page.mutedColor
 font.pixelSize: 13 } } } }
 Item { Layout.fillHeight: true }
        } }
    }
}
}
}

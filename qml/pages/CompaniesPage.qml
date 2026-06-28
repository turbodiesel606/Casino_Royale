import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page
    property int selectedRow: 0
    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property var companies: [["KDAB", "kdab.com", "2 jobs", "3 contacts", "2 days ago"], ["TechSoft", "techsoft.com", "4 jobs", "5 contacts", "5 days ago"], ["Vision Systems", "visionsystems.com", "1 job", "2 contacts", "1 week ago"], ["CodeVision", "codevision.io", "0 jobs", "1 contact", "2 weeks ago"], ["Bosch", "bosch.com", "3 jobs", "7 contacts", "3 weeks ago"]]
    RowLayout { anchors.fill: parent
 spacing: 0
        Item { Layout.fillWidth: true
 Layout.fillHeight: true
 Layout.leftMargin: 27
 Layout.topMargin: 25
 Layout.bottomMargin: 34
            ColumnLayout { anchors.fill: parent
 spacing: 16
                Text { text: "Companies"
 color: page.textColor
 font.pixelSize: 33
 font.bold: true }
                RowLayout { Layout.fillWidth: true
 TextField { Layout.preferredWidth: 335
 Layout.preferredHeight: 43
 placeholderText: "⌕   Search companies..."
 color: page.textColor
 placeholderTextColor: page.mutedColor
 background: Rectangle { color: "#0b1b27"
 border.color: "#273b49"
 radius: 7 } }
 Button { text: "⚱"
 Layout.preferredWidth: 45
 Layout.preferredHeight: 43
 contentItem: Text { text: parent.text
 color: page.textColor
 font.pixelSize: 19
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter }
 background: Rectangle { color: "#0e1e2a"
 border.color: "#2c3e4a"
 radius: 6 } }
 Item { Layout.fillWidth: true }
 Button { text: "+  Add Company"
 Layout.preferredHeight: 43
 Layout.preferredWidth: 153
 contentItem: Text { text: parent.text
 color: "white"
 font.pixelSize: 15
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter }
 background: Rectangle { color: "#1479ee"
 radius: 6 } } }
                Panel { Layout.fillWidth: true
 Layout.fillHeight: true
 ColumnLayout { anchors.fill: parent
 spacing: 0
                    RowLayout { Layout.fillWidth: true
 Layout.preferredHeight: 58
 Layout.leftMargin: 18
 spacing: 0
 Repeater { model: [["Company  ↑", 265], ["Website", 190], ["Open Jobs", 160], ["Contacts", 150], ["Last Activity  ↕", 130]]
 delegate: Text { required property var modelData
 text: modelData[0]
 color: page.textColor
 font.pixelSize: 15
 Layout.preferredWidth: modelData[1] } } }
                    Repeater { model: page.companies
 delegate: Rectangle { required property var modelData
 required property int index
 Layout.fillWidth: true
 Layout.preferredHeight: 88
 color: index === page.selectedRow ? "#123757" : (mouse.containsMouse ? "#0e2738" : "transparent")
 border.color: index === page.selectedRow ? "#1687ff" : "#263945"
 border.width: index === page.selectedRow ? 1 : 0
 MouseArea { id: mouse
 anchors.fill: parent
 hoverEnabled: true
 onClicked: page.selectedRow = index }
 RowLayout { anchors.fill: parent
 anchors.leftMargin: 18
 spacing: 0
 RowLayout { Layout.preferredWidth: 265
 spacing: 14
 Rectangle { width: 55
 height: 55
 radius: 6
 color: index === 0 ? "#146ce0" : "#10273a"
 Text { anchors.centerIn: parent
 text: index === 0 ? "KDAB" : modelData[0].slice(0, 2)
 color: "white"
 font.pixelSize: index === 0 ? 16 : 19
 font.bold: true } }
 Text { text: modelData[0]
 color: page.textColor
 font.pixelSize: 17
 font.bold: index === 0 } }
 Text { text: modelData[1]
 color: "#2b91ff"
 font.pixelSize: 15
 Layout.preferredWidth: 190 }
 Text { text: modelData[2]
 color: "#2b91ff"
 font.pixelSize: 15
 Layout.preferredWidth: 160 }
 Text { text: modelData[3]
 color: "#2b91ff"
 font.pixelSize: 15
 Layout.preferredWidth: 150 }
 Text { text: modelData[4]
 color: page.mutedColor
 font.pixelSize: 15
 Layout.preferredWidth: 130 } } } }
                    Item { Layout.fillHeight: true }
 Text { text: "Showing 1 to 5 of 5 companies"
 color: page.mutedColor
 font.pixelSize: 14
 Layout.leftMargin: 18
 Layout.bottomMargin: 18 }
                } }
            }
        }
        Panel { Layout.preferredWidth: 455
 Layout.fillHeight: true
 radius: 0
 ColumnLayout { anchors.fill: parent
 anchors.margins: 33
 spacing: 14
            Text { text: "Preview"
 color: page.textColor
 font.pixelSize: 19
 font.bold: true }
 Rectangle { Layout.fillWidth: true
 height: 1
 color: "#2a3b46" }
            Text { text: "Company Details"
 color: page.textColor
 font.pixelSize: 18
 font.bold: true }
            RowLayout { Rectangle { width: 68
 height: 68
 radius: 6
 color: "#146ce0"
 Text { anchors.centerIn: parent
 text: "KDAB"
 color: "white"
 font.pixelSize: 18
 font.bold: true } }
 ColumnLayout { Text { text: page.companies[page.selectedRow][0]
 color: page.textColor
 font.pixelSize: 24
 font.bold: true }
 Text { text: "◉  " + page.companies[page.selectedRow][1]
 color: "#2588ff"
 font.pixelSize: 15 } } }
            Text { text: "Description\nKDAB is the leading software consultancy for Qt, C++, and 3D technologies. We help customers deliver high-performance cross-platform solutions and provide training and support for developers."
 color: page.textColor
 font.pixelSize: 15
 wrapMode: Text.WordWrap
 lineHeight: 1.25 }
            Text { text: "Saved Notes"
 color: page.textColor
 font.pixelSize: 16
 font.bold: true }
 Rectangle { Layout.fillWidth: true
 Layout.preferredHeight: 84
 radius: 7
 color: "#0c1b26"
 border.color: "#2a3b46"
 Text { anchors.fill: parent
 anchors.margins: 12
 text: "Strong focus on Qt and C++ expertise, great engineering culture and active in the Qt community."
 color: page.mutedColor
 wrapMode: Text.WordWrap
 font.pixelSize: 14 } }
            Text { text: "Linked Jobs   2                                  View all jobs"
 color: page.textColor
 font.pixelSize: 16
 font.bold: true
 Layout.topMargin: 10 }
 Repeater { model: ["▣   C++/Qt Developer                 Applied", "▣   Qt/QML Engineer                 Interview"]
 delegate: Rectangle { required property string modelData
 Layout.fillWidth: true
 Layout.preferredHeight: 64
 radius: 7
 color: "#0d1b25"
 border.color: "#2b3b45"
 Text { anchors.fill: parent
 anchors.margins: 12
 text: modelData
 color: page.textColor
 verticalAlignment: Text.AlignVCenter
 font.pixelSize: 14 } } }
            Text { text: "Employees   3                              View all contacts"
 color: page.textColor
 font.pixelSize: 16
 font.bold: true
 Layout.topMargin: 10 }
 Repeater { model: ["Anna Müller       HR Manager", "Thomas Becker       Senior Software Engineer", "Julia Schneider       Talent Acquisition Specialist"]
 delegate: Text { required property string modelData
 text: "●   " + modelData + "    in   ✉"
 color: page.textColor
 font.pixelSize: 15
 Layout.fillWidth: true
 Layout.preferredHeight: 38 } }
 Item { Layout.fillHeight: true }
        } }
    }
}

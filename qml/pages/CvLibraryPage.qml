import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page
    property int selectedRow: 0
    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property var cvs: [["CV_Qt_2026.pdf", "Qt/QML Engineer", "Qt/QML Developer", "May 12, 2026", "8"], ["CV_Cpp_Developer.pdf", "C++ Developer", "C++ Developer", "May 5, 2026", "5"], ["CV_Remote.pdf", "Remote Software Engineer", "English CV", "Apr 28, 2026", "3"], ["CV_Office.pdf", "Office Administrator", "Office Job", "Apr 18, 2026", "2"]]
    RowLayout { anchors.fill: parent
 spacing: 12
        Item { Layout.fillWidth: true
 Layout.fillHeight: true
 Layout.leftMargin: 30
 Layout.topMargin: 21
 Layout.bottomMargin: 28
            ColumnLayout { anchors.fill: parent
 spacing: 12
                RowLayout { Layout.fillWidth: true
 Text { text: "CV Library"
 color: page.textColor
 font.pixelSize: 32
 font.bold: true }
 Item { Layout.fillWidth: true }
 TextField { Layout.preferredWidth: 485
 Layout.preferredHeight: 47
 placeholderText: "⌕   Search CVs..."
 color: page.textColor
 placeholderTextColor: page.mutedColor
 background: Rectangle { color: "#0b1b27"
 border.color: "#273b49"
 radius: 7 } } }
                Panel { Layout.fillWidth: true
 Layout.preferredHeight: 101
 RowLayout { anchors.fill: parent
 anchors.margins: 17
 spacing: 21
 Repeater { model: [["Category", "All"], ["Language", "All"], ["Last Modified", "Any time"], ["Used in Jobs", "Any"]]
 delegate: Rectangle { required property var modelData
 width: 185
 height: 65
 color: "#111e28"
 border.color: "#2c3e4a"
 radius: 7
 Column { anchors.fill: parent
 anchors.margins: 11
 Text { text: modelData[0]
 color: page.mutedColor
 font.pixelSize: 14 }
 Text { text: modelData[1] + "                         ⌄"
 color: page.textColor
 font.pixelSize: 16
 topPadding: 7 } } } }
 Item { Layout.fillWidth: true }
 Text { text: "⚙    Reset"
 color: "#2688ff"
 font.pixelSize: 15 } } }
                RowLayout { Layout.fillWidth: true
 Layout.fillHeight: true
 spacing: 12
                    Panel { Layout.preferredWidth: 220
 Layout.fillHeight: true
 ColumnLayout { anchors.fill: parent
 anchors.margins: 13
 Text { text: "Categories"
 color: page.textColor
 font.pixelSize: 18
 font.bold: true
 Layout.bottomMargin: 8 }
 Repeater { model: [["All CVs", "7"], ["C++ Developer", "2"], ["Qt/QML Developer", "2"], ["Flutter Developer", "1"], ["1C Developer", "1"], ["Office Job", "1"], ["English CV", "5"], ["Russian CV", "2"]]
 delegate: Rectangle { required property var modelData
 Layout.fillWidth: true
 Layout.preferredHeight: 42
 radius: 6
 color: index === 0 ? "#153c69" : "transparent"
 RowLayout { anchors.fill: parent
 anchors.margins: 8
 Text { text: modelData[0]
 color: index === 0 ? "#60a9ff" : page.textColor
 font.pixelSize: 15
 Layout.fillWidth: true }
 Text { text: modelData[1]
 color: page.mutedColor
 font.pixelSize: 13 } } } }
 Item { Layout.fillHeight: true }
 Button { Layout.fillWidth: true
 text: "⚙  Manage Categories"
 contentItem: Text { text: parent.text
 color: "#2688ff"
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter }
 background: Rectangle { color: "#10212e"
 border.color: "#2a3c49"
 radius: 6 } } } }
                    ColumnLayout { Layout.fillWidth: true
 Layout.fillHeight: true
 spacing: 10
                        Panel { Layout.fillWidth: true
 Layout.preferredHeight: 60
 RowLayout { anchors.fill: parent
 anchors.margins: 15
 Text { text: "7 CVs"
 color: page.mutedColor
 font.pixelSize: 15 }
 Item { Layout.fillWidth: true }
 Text { text: "Sort by:  Last Modified   ⌄"
 color: page.mutedColor
 font.pixelSize: 15 } } }
                        Repeater { model: page.cvs
 delegate: Panel { required property var modelData
 required property int index
 Layout.fillWidth: true
 Layout.preferredHeight: 133
 border.color: index === page.selectedRow ? "#1687ff" : "#223542"
 MouseArea { anchors.fill: parent
 onClicked: page.selectedRow = index }
 RowLayout { anchors.fill: parent
 anchors.margins: 20
 spacing: 18
 Rectangle { width: 66
 height: 85
 color: "#f6f6f5"
 Text { anchors.centerIn: parent
 text: "PDF"
 color: "#d53e2d"
 font.bold: true } }
 ColumnLayout { Layout.preferredWidth: 260
 Text { text: modelData[0]
 color: page.textColor
 font.pixelSize: 21
 font.bold: true }
 Text { text: modelData[1]
 color: page.textColor
 font.pixelSize: 16 }
 StatusChip { label: modelData[2]
 accent: modelData[2] === "Office Job" ? "#ffbd21" : "#1687ff" } }
 Rectangle { width: 1
 Layout.fillHeight: true
 color: "#2c3b45" }
 ColumnLayout { Text { text: "▣  Updated " + modelData[3]
 color: page.mutedColor
 font.pixelSize: 15 }
 Text { text: "♧  Used in " + modelData[4] + " jobs"
 color: page.mutedColor
 font.pixelSize: 15 } }
 Item { Layout.fillWidth: true }
 Button { text: "Open Details"
 contentItem: Text { text: parent.text
 color: "#2a91ff"
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter }
 background: Rectangle { color: "transparent"
 border.color: "#167ce9"
 radius: 5 } } } } }
                    }
                }
            }
        }
        Panel { Layout.preferredWidth: 405
 Layout.fillHeight: true
 radius: 0
 ColumnLayout { anchors.fill: parent
 anchors.margins: 16
 Text { text: "Preview"
 color: page.textColor
 font.pixelSize: 18
 font.bold: true }
 Rectangle { Layout.fillWidth: true
 height: 1
 color: "#2a3b46" }
 RowLayout { Rectangle { width: 74
 height: 91
 color: "#f6f6f5"
 Text { anchors.centerIn: parent
 text: "PDF"
 color: "#d53e2d"
 font.bold: true } }
 ColumnLayout { Text { text: page.cvs[page.selectedRow][0]
 color: page.textColor
 font.pixelSize: 21
 font.bold: true }
 Text { text: page.cvs[page.selectedRow][1]
 color: page.mutedColor
 font.pixelSize: 15 }
 StatusChip { label: page.cvs[page.selectedRow][2]
 accent: "#1687ff" } } }
 Text { text: "Description\nCV focused on Qt/QML development, desktop applications, and cross-platform experience."
 color: page.textColor
 font.pixelSize: 15
 wrapMode: Text.WordWrap
 Layout.topMargin: 20 }
 Repeater { model: [["Language", "English"], ["Last Modified", "May 12, 2026"], ["File Size", "612 KB"], ["Used in Jobs", "8"]]
 delegate: RowLayout { required property var modelData
 Layout.fillWidth: true
 Text { text: modelData[0]
 color: page.mutedColor
 font.pixelSize: 15
 Layout.preferredWidth: 130 }
 Text { text: modelData[1]
 color: page.textColor
 font.pixelSize: 15 } } }
 Text { text: "Linked Applications (8)                    View all"
 color: page.textColor
 font.pixelSize: 16
 font.bold: true
 Layout.topMargin: 20 }
 Repeater { model: ["Senior Qt/QML Developer     Interview", "Qt Developer (Desktop)     Applied", "Qt/QML Engineer     Screening"]
 delegate: Rectangle { required property string modelData
 Layout.fillWidth: true
 Layout.preferredHeight: 64
 color: "#0d1b25"
 border.color: "#2c3b45"
 radius: 7
 Text { anchors.fill: parent
 anchors.margins: 12
 text: "▣   " + modelData
 color: page.textColor
 font.pixelSize: 14
 verticalAlignment: Text.AlignVCenter } } }
 Item { Layout.fillHeight: true } } }
    }
}

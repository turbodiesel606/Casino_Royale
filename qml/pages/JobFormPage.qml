import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page
    signal cancelRequested()
    signal saveRequested()
    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"

    RowLayout { anchors.fill: parent
 spacing: 18
        Flickable { Layout.fillWidth: true
 Layout.fillHeight: true
 Layout.leftMargin: 28
 Layout.topMargin: 28
 Layout.bottomMargin: 25
 clip: true
 contentWidth: width
 contentHeight: form.implicitHeight
            ColumnLayout { id: form
 width: parent.width
 spacing: 12
                Text { text: "Job Applications    /    Add Job Application"
 color: "#2688ff"
 font.pixelSize: 15 }
                Text { text: "Add Job Application"
 color: page.textColor
 font.pixelSize: 37
 font.bold: true
 Layout.bottomMargin: 1 }
                Panel { Layout.fillWidth: true
 Layout.preferredHeight: 796
                    GridLayout { anchors.fill: parent
 anchors.margins: 18
 columns: 2
 columnSpacing: 20
 rowSpacing: 14
                        Repeater { model: [["Job Title", "C++/Qt Developer"], ["Job URL", "https://example.com/jobs/cpp-qt-developer"], ["Company", "Example Corp."], ["Work Format", "⌂  Remote                         ⌄"], ["City", "Prague, Czech Republic"], ["Salary", "€ 70,000 – € 90,000"]]
                            delegate: ColumnLayout { required property var modelData
 Layout.fillWidth: true
 spacing: 7
                                Text { text: modelData[0]
 color: page.textColor
 font.pixelSize: 15 }
                                TextField { Layout.fillWidth: true
 Layout.preferredHeight: 40
 text: modelData[1]
 color: page.textColor
 font.pixelSize: 15
 background: Rectangle { radius: 6
 color: "#101b22"
 border.color: "#2d3b45" } }
                            }
                        }
                        Repeater { model: [["Description", "We are looking for an experienced C++/Qt Developer to build cross-platform desktop applications used by millions of users. You will work on modern codebases, collaborate in an agile team, and contribute to the entire development lifecycle."], ["Requirements", "• 5+ years of C++ development experience\n• Strong knowledge of Qt (Widgets, QML)\n• Experience with CMake and modern C++ (C++17/20)\n• Understanding of multi-threading and async programming\n• Good communication skills in English"]]
                            delegate: ColumnLayout { required property var modelData
 Layout.fillWidth: true
 spacing: 7
                                Text { text: modelData[0]
 color: page.textColor
 font.pixelSize: 15 }
                                TextArea { Layout.fillWidth: true
 Layout.preferredHeight: 135
 text: modelData[1]
 wrapMode: TextArea.Wrap
 color: page.textColor
 font.pixelSize: 15
 background: Rectangle { radius: 6
 color: "#101b22"
 border.color: "#2d3b45" } }
                            }
                        }
                        ColumnLayout { Layout.columnSpan: 2
 Layout.fillWidth: true
 spacing: 7
                            Text { text: "Tech Stack"
 color: page.textColor
 font.pixelSize: 15 }
                            Rectangle { Layout.fillWidth: true
 Layout.preferredHeight: 49
 radius: 6
 color: "#101b22"
 border.color: "#2d3b45"
 RowLayout { anchors.fill: parent
 anchors.margins: 10
 spacing: 9
                                Repeater { model: ["C++  ×", "Qt  ×", "QML  ×", "CMake  ×"]
 delegate: Rectangle { required property string modelData
 radius: 18
 height: 30
 width: 78
 color: "#16467a"
 Text { anchors.centerIn: parent
 text: modelData
 color: "#dbe9ff"
 font.pixelSize: 14 } } }
                                Text { text: "Add technology..."
 color: page.mutedColor
 font.pixelSize: 14 }
                            } }
                        }
                        ColumnLayout { Layout.columnSpan: 2
 Layout.fillWidth: true
 spacing: 7
                            Text { text: "Notes"
 color: page.textColor
 font.pixelSize: 15 }
                            TextArea { Layout.fillWidth: true
 Layout.preferredHeight: 94
 text: "Applied via company careers page. The team focuses on cross-platform desktop tools.\nFollow up in 1 week if no response."
 color: page.textColor
 wrapMode: TextArea.Wrap
 font.pixelSize: 15
 background: Rectangle { radius: 6
 color: "#101b22"
 border.color: "#2d3b45" } }
                        }
                        RowLayout { Layout.columnSpan: 2
 Layout.fillWidth: true
 Layout.topMargin: 17
                            Button { text: "Cancel"
 onClicked: page.cancelRequested()
 Layout.preferredHeight: 46
 contentItem: Text { text: parent.text
 color: page.textColor
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter
 font.pixelSize: 16 }
 background: Rectangle { radius: 6
 color: "#252d34"
 border.color: "#35404a" } }
                            Item { Layout.fillWidth: true }
                            Button { text: "Save"
 onClicked: page.saveRequested()
 Layout.preferredWidth: 94
 Layout.preferredHeight: 46
 contentItem: Text { text: parent.text
 color: "white"
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter
 font.pixelSize: 16 }
 background: Rectangle { radius: 6
 color: "#1479ee" } }
                        }
                    }
                }
            }
        }
        ColumnLayout { Layout.preferredWidth: 395
 Layout.fillHeight: true
 Layout.rightMargin: 28
 Layout.topMargin: 48
 Layout.bottomMargin: 25
 spacing: 12
            Panel { Layout.fillWidth: true
 Layout.preferredHeight: 368
 ColumnLayout { anchors.fill: parent
 anchors.margins: 16
 spacing: 0
 Text { text: "Status"
 color: page.textColor
 font.pixelSize: 19
 font.bold: true
 Layout.bottomMargin: 10 }
                Repeater { model: [["Draft", "#7f868b"], ["Applied", "#2384ff"], ["Interview", "#ffbd27"], ["Test Task", "#a372f2"], ["Offer", "#49c768"], ["Rejected", "#fb4d4a"], ["Archived", "#7f868b"]]
                    delegate: Rectangle { required property var modelData
 Layout.fillWidth: true
 Layout.preferredHeight: 44
 color: index === 0 ? "#242b30" : "transparent"
 border.color: "#2c3941"
 RowLayout { anchors.fill: parent
 anchors.margins: 13
 Rectangle { width: 16
 height: 16
 radius: 8
 color: modelData[1] }
 Text { text: modelData[0]
 color: page.textColor
 font.pixelSize: 16
 Layout.fillWidth: true }
 Text { text: index === 0 ? "◉" : "○"
 color: index === 0 ? "#2688ff" : "#9aa6af"
 font.pixelSize: 21 } } }
                }
            } }
            Panel { Layout.fillWidth: true
 Layout.preferredHeight: 138
 ColumnLayout { anchors.fill: parent
 anchors.margins: 16
 Text { text: "CV used"
 color: page.textColor
 font.pixelSize: 19
 font.bold: true }
 RowLayout { Layout.fillWidth: true
 Rectangle { width: 60
 height: 68
 color: "#e8edf1"
 Text { anchors.centerIn: parent
 text: "PDF"
 color: "#d63d2c"
 font.bold: true } }
 ColumnLayout { Text { text: "CV_Qt_2026.pdf"
 color: page.textColor
 font.pixelSize: 17 }
 Button { text: "Choose CV"
 contentItem: Text { text: parent.text
 color: page.textColor
 horizontalAlignment: Text.AlignHCenter
 verticalAlignment: Text.AlignVCenter }
 background: Rectangle { color: "#1c252c"
 border.color: "#37434c"
 radius: 5 } } } } } }
            Panel { Layout.fillWidth: true
 Layout.preferredHeight: 100
 ColumnLayout { anchors.fill: parent
 anchors.margins: 16
 Text { text: "Application Date"
 color: page.textColor
 font.pixelSize: 18
 font.bold: true }
 Text { text: "▣   May 12, 2026                         ⌄"
 color: page.textColor
 font.pixelSize: 16 } } }
            Panel { Layout.fillWidth: true
 Layout.preferredHeight: 135
 ColumnLayout { anchors.fill: parent
 anchors.margins: 16
 Text { text: "Additional Files"
 color: page.textColor
 font.pixelSize: 18
 font.bold: true }
 Rectangle { Layout.fillWidth: true
 Layout.fillHeight: true
 color: "transparent"
 border.color: "#4a565e"
 radius: 6
 Text { anchors.centerIn: parent
 text: "⌕  Attach file\n\nDrag and drop files here or click to attach"
 color: page.mutedColor
 horizontalAlignment: Text.AlignHCenter
 font.pixelSize: 14 } } } }
            Panel { Layout.fillWidth: true
 Layout.preferredHeight: 95
 ColumnLayout { anchors.fill: parent
 anchors.margins: 16
 Text { text: "Contacts"
 color: page.textColor
 font.pixelSize: 18
 font.bold: true }
 Text { text: "♙  Add Contact    ⌄"
 color: page.textColor
 font.pixelSize: 15 } } }
            Item { Layout.fillHeight: true }
        }
    }
}

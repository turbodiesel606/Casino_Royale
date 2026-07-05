import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: page
    clip: true

    readonly property real uiScale: 0.80
    readonly property color bgColor: "#07131d"
    readonly property color panelColor: "#0b1b27"
    readonly property color panelSoftColor: "#0e202d"
    readonly property color lineColor: "#243746"
    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property color blueColor: "#1687ff"
    readonly property color greenColor: "#59d34d"
    readonly property color yellowColor: "#ffbd21"
    readonly property color purpleColor: "#b36bff"
    readonly property var selectedCv: cvLibraryController.selectedCv

    Item {
        id: scaledContent
        width: page.width / page.uiScale
        height: page.height / page.uiScale
        scale: page.uiScale
        transformOrigin: Item.TopLeft

        Rectangle {
            anchors.fill: parent
            color: page.bgColor
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 26
            anchors.rightMargin: 20
            anchors.topMargin: 22
            anchors.bottomMargin: 30
            spacing: 20

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 46
                spacing: 18

                Text {
                    text: "CV Library"
                    color: page.textColor
                    font.pixelSize: 31
                    font.bold: true
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                }

                CvSearchField {
                    Layout.preferredWidth: 485
                    Layout.preferredHeight: 46
                    text: cvLibraryController.searchText
                    textColor: page.textColor
                    mutedColor: page.mutedColor
                    lineColor: page.lineColor
                    onTextChanged: {
                        if (text !== cvLibraryController.searchText)
                            cvLibraryController.setSearchText(text)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                CvLibraryBrowserPane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    cvModel: cvLibraryController.cvModel
                    categorySummary: cvLibraryController.categorySummary
                    resultSummary: cvLibraryController.resultSummary
                    categoryFilter: cvLibraryController.categoryFilter
                    languageFilter: cvLibraryController.languageFilter
                    sortMode: cvLibraryController.sortMode
                    selectedRow: cvLibraryController.selectedCvIndex
                    panelColor: page.panelColor
                    lineColor: page.lineColor
                    textColor: page.textColor
                    mutedColor: page.mutedColor
                    blueColor: page.blueColor
                    greenColor: page.greenColor
                    yellowColor: page.yellowColor
                    purpleColor: page.purpleColor
                    onRowSelected: row => cvLibraryController.selectCv(row)
                    onCategoryFilterRequested: category => cvLibraryController.setCategoryFilter(category)
                    onLanguageFilterRequested: language => cvLibraryController.setLanguageFilter(language)
                    onSortModeRequested: mode => cvLibraryController.setSortMode(mode)
                    onClearFiltersRequested: cvLibraryController.clearFilters()
                }

                CvLibraryPreviewPanel {
                    Layout.preferredWidth: 405
                    Layout.fillHeight: true
                    selectedCv: page.selectedCv
                    linkedApplicationsModel: cvLibraryController.linkedApplicationsModel
                    panelColor: page.panelColor
                    lineColor: page.lineColor
                    textColor: page.textColor
                    mutedColor: page.mutedColor
                    blueColor: page.blueColor
                    greenColor: page.greenColor
                    yellowColor: page.yellowColor
                    purpleColor: page.purpleColor
                    onFavoriteToggled: cvLibraryController.toggleFavorite(page.selectedCv.id)
                    onOpenCvRequested: cvLibraryController.openCv(page.selectedCv.id)
                }
            }
        }
    }
}

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

                RowLayout {
                    spacing: 0

                    Button {
                        Layout.preferredWidth: 104
                        Layout.preferredHeight: 40
                        text: "Active"
                        checked: cvLibraryController.libraryView === 0
                        onClicked: cvLibraryController.setLibraryView(0)

                        contentItem: Text {
                            text: parent.text
                            color: parent.checked ? "white" : page.mutedColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 14
                            font.bold: parent.checked
                        }

                        background: Rectangle {
                            radius: 6
                            color: parent.checked ? page.blueColor : page.panelSoftColor
                            border.color: parent.checked ? "#59adff" : page.lineColor
                        }
                    }

                    Button {
                        Layout.preferredWidth: 104
                        Layout.preferredHeight: 40
                        text: "Archived"
                        checked: cvLibraryController.libraryView === 1
                        onClicked: cvLibraryController.setLibraryView(1)

                        contentItem: Text {
                            text: parent.text
                            color: parent.checked ? "white" : page.mutedColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 14
                            font.bold: parent.checked
                        }

                        background: Rectangle {
                            radius: 6
                            color: parent.checked ? page.blueColor : page.panelSoftColor
                            border.color: parent.checked ? "#59adff" : page.lineColor
                        }
                    }
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
                    archivedView: cvLibraryController.libraryView === 1
                    checkedCvIds: cvLibraryController.checkedCvIds
                    checkedLinkedCvCount: cvLibraryController.checkedLinkedCvCount
                    checkedUnlinkedCvCount: cvLibraryController.checkedUnlinkedCvCount
                    allVisibleCvsChecked: cvLibraryController.allVisibleCvsChecked
                    someVisibleCvsChecked: cvLibraryController.someVisibleCvsChecked
                    mutationEnabled: cvLibraryController.canMutateCheckedCvs
                    mutationBusy: cvLibraryController.mutatingCvs
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
                    onRowCheckToggled: row => cvLibraryController.toggleCvChecked(row)
                    onAllVisibleCheckedRequested: checked => cvLibraryController.setAllVisibleCvsChecked(checked)
                    onRemoveRequested: removeConfirmation.open()
                    onRestoreRequested: cvLibraryController.restoreCheckedCvs()
                    onPermanentDeleteRequested: permanentDeleteConfirmation.open()
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

    DestructiveConfirmationDialog {
        id: removeConfirmation
        anchors.centerIn: parent
        title: "Remove CVs from the library"
        confirmText: "Remove Selected"
        message: "Remove " + cvLibraryController.checkedCvCount + " selected CV(s)? "
            + cvLibraryController.checkedLinkedCvCount + " linked CV(s) will be archived and their files kept. "
            + cvLibraryController.checkedUnlinkedCvCount + " unlinked CV(s) and their managed files will be permanently deleted."
        onConfirmed: cvLibraryController.removeCheckedCvs()
    }

    DestructiveConfirmationDialog {
        id: permanentDeleteConfirmation
        anchors.centerIn: parent
        title: "Permanently delete archived CVs"
        confirmText: "Delete Permanently"
        message: "Permanently delete " + cvLibraryController.checkedUnlinkedCvCount
            + " unlinked archived CV(s) and their managed files? "
            + cvLibraryController.checkedLinkedCvCount
            + " linked CV(s) will be skipped and remain archived. This action cannot be undone."
        onConfirmed: cvLibraryController.permanentlyDeleteCheckedCvs()
    }
}

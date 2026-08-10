import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Item {
    id: page

    property url selectedCvUrl: ""
    property var fieldErrors: ({})
    property string saveError: ""
    property bool admissionLocked: false
    property var admissionOperationId: null

    readonly property color textColor: "#eef3f8"
    readonly property color mutedColor: "#a8b5c2"
    readonly property color fieldColor: "#071925"
    readonly property color lineColor: "#223542"
    readonly property color accentBlue: "#1479ee"
    readonly property int scrollbarWidth: 11
    readonly property int scrollbarGap: 12

    function errorFor(fieldName) {
        return fieldErrors && fieldErrors[fieldName] ? fieldErrors[fieldName] : ""
    }

    function resetForm() {
        jobTitleField.text = ""
        jobUrlField.text = ""
        companyField.text = ""
        workFormatField.text = ""
        cityField.text = ""
        salaryField.text = ""
        statusField.text = "Applied"
        appliedDateField.text = ""
        nextStepField.text = ""
        descriptionField.text = ""
        requirementsField.text = ""
        techStackField.text = ""
        notesField.text = ""
        selectedCvUrl = ""
        fieldErrors = ({})
        saveError = ""
    }

    function submit() {
        saveError = ""
        jobApplicationsController.createApplication({
            jobTitle: jobTitleField.text,
            jobUrl: jobUrlField.text,
            companyName: companyField.text,
            workFormat: workFormatField.text,
            city: cityField.text,
            salary: salaryField.text,
            status: statusField.text,
            appliedDate: appliedDateField.text,
            nextStep: nextStepField.text,
            description: descriptionField.text,
            requirements: requirementsField.text,
            techStack: techStackField.text,
            notes: notesField.text
        }, selectedCvUrl)
    }

    component FieldLabel: Text {
        color: page.textColor
        font.pixelSize: 15
    }

    component FormField: TextField {
        property string errorText: ""

        enabled: !page.admissionLocked
        color: page.textColor
        font.pixelSize: 15
        leftPadding: 14
        rightPadding: 14
        selectByMouse: true

        background: Rectangle {
            color: page.fieldColor
            border.color: parent.errorText.length > 0 ? "#ff4b49" : page.lineColor
            radius: 6
        }
    }

    component FormArea: TextArea {
        enabled: !page.admissionLocked
        color: page.textColor
        font.pixelSize: 15
        leftPadding: 14
        rightPadding: 14
        topPadding: 12
        bottomPadding: 12
        wrapMode: TextArea.Wrap
        selectByMouse: true

        background: Rectangle {
            color: page.fieldColor
            border.color: page.lineColor
            radius: 6
        }
    }

    component FormError: Text {
        required property string message

        visible: message.length > 0
        text: message
        color: "#ff6b69"
        font.pixelSize: 12
        wrapMode: Text.Wrap
    }

    FileDialog {
        id: cvFileDialog
        title: "Select CV"
        nameFilters: ["CV documents (*.pdf *.doc *.docx)"]
        onAccepted: {
            page.selectedCvUrl = selectedFile
            page.fieldErrors = ({})
            page.saveError = ""
        }
    }

    Connections {
        target: jobApplicationsController

        function onApplicationQueued(operationId) {
            page.fieldErrors = ({})
            page.saveError = ""
            page.admissionOperationId = operationId
            page.admissionLocked = true
        }

        function onApplicationAccepted(operationId, jobTitle) {
            if (!page.admissionLocked || page.admissionOperationId !== operationId)
                return

            page.resetForm()
            page.admissionOperationId = null
            page.admissionLocked = false
        }

        function onApplicationRejected(operationId, errors, message) {
            if (!page.admissionLocked || page.admissionOperationId !== operationId)
                return

            page.fieldErrors = errors
            page.saveError = message
            page.admissionOperationId = null
            page.admissionLocked = false
        }
    }

    Panel {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 22
        width: Math.min(parent.width - 44, 930)
        height: parent.height - 44
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 42

                Text {
                    text: "Add new Job"
                    color: page.textColor
                    font.pixelSize: 24
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    Layout.preferredWidth: 104
                    Layout.preferredHeight: 42
                    enabled: !page.admissionLocked
                    text: "Save"
                    onClicked: page.submit()

                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15
                    }

                    background: Rectangle {
                        color: parent.enabled ? page.accentBlue : "#31506d"
                        radius: 6
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Flickable {
                    id: formFlickable
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: formScrollbarSeparator.left
                    anchors.rightMargin: page.scrollbarGap
                    clip: true
                    contentWidth: width
                    contentHeight: formContent.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds

                    ColumnLayout {
                        id: formContent
                        width: formFlickable.width
                        spacing: 12

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 22
                        rowSpacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            FieldLabel { text: "Job Title" }
                            FormField {
                                id: jobTitleField
                                Layout.fillWidth: true
                                placeholderText: "Job title"
                                errorText: page.errorFor("jobTitle")
                            }
                            FormError { message: jobTitleField.errorText }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            FieldLabel { text: "Job URL" }
                            FormField {
                                id: jobUrlField
                                Layout.fillWidth: true
                                placeholderText: "https://..."
                                errorText: page.errorFor("jobUrl")
                            }
                            FormError { message: jobUrlField.errorText }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            FieldLabel { text: "Company" }
                            FormField {
                                id: companyField
                                Layout.fillWidth: true
                                placeholderText: "Company"
                                errorText: page.errorFor("companyName")
                            }
                            FormError { message: companyField.errorText }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            FieldLabel { text: "Work Format" }
                            FormField {
                                id: workFormatField
                                Layout.fillWidth: true
                                placeholderText: "Remote, Hybrid, On-site"
                                errorText: page.errorFor("workFormat")
                            }
                            FormError { message: workFormatField.errorText }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            FieldLabel { text: "City" }
                            FormField {
                                id: cityField
                                Layout.fillWidth: true
                                placeholderText: "City"
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            FieldLabel { text: "Salary" }
                            FormField {
                                id: salaryField
                                Layout.fillWidth: true
                                placeholderText: "Salary"
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            FieldLabel { text: "Status" }
                            FormField {
                                id: statusField
                                Layout.fillWidth: true
                                text: "Applied"
                                errorText: page.errorFor("status")
                            }
                            FormError { message: statusField.errorText }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            FieldLabel { text: "Application Date" }
                            FormField {
                                id: appliedDateField
                                Layout.fillWidth: true
                                placeholderText: "yyyy-MM-dd (today if empty)"
                                errorText: page.errorFor("appliedDate")
                            }
                            FormError { message: appliedDateField.errorText }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.columnSpan: 2
                            FieldLabel { text: "Next Step" }
                            FormField {
                                id: nextStepField
                                Layout.fillWidth: true
                                placeholderText: "Optional next action"
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        FieldLabel { text: "CV used" }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 49
                            color: page.fieldColor
                            border.color: page.errorFor("cv").length > 0 ? "#ff4b49" : page.lineColor
                            radius: 6

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 16
                                anchors.rightMargin: 12

                                Text {
                                    Layout.fillWidth: true
                                    text: page.selectedCvUrl.toString().length > 0
                                        ? decodeURIComponent(page.selectedCvUrl.toString().split("/").pop())
                                        : "No CV selected"
                                    color: page.selectedCvUrl.toString().length > 0 ? page.textColor : page.mutedColor
                                    elide: Text.ElideMiddle
                                    font.pixelSize: 15
                                }

                                Button {
                                    enabled: !page.admissionLocked
                                    text: page.selectedCvUrl.toString().length > 0 ? "Change CV" : "Select CV"
                                    onClicked: cvFileDialog.open()
                                }
                            }
                        }

                        FormError { message: page.errorFor("cv") }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 22

                        ColumnLayout {
                            Layout.fillWidth: true
                            FieldLabel { text: "Description" }
                            FormArea {
                                id: descriptionField
                                Layout.fillWidth: true
                                Layout.preferredHeight: 90
                                placeholderText: "Job description"
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            FieldLabel { text: "Requirements" }
                            FormArea {
                                id: requirementsField
                                Layout.fillWidth: true
                                Layout.preferredHeight: 90
                                placeholderText: "Requirements"
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        FieldLabel { text: "Tech Stack" }
                        FormField {
                            id: techStackField
                            Layout.fillWidth: true
                            placeholderText: "C++, Qt, QML"
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        FieldLabel { text: "Notes" }
                        FormArea {
                            id: notesField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 70
                            placeholderText: "Notes"
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: page.saveError.length > 0
                        text: page.saveError
                        color: "#ff6b69"
                        font.pixelSize: 13
                        wrapMode: Text.Wrap
                    }
                }
                }

                Rectangle {
                    id: formScrollbarSeparator
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: formVerticalScrollBar.left
                    anchors.rightMargin: page.scrollbarGap
                    width: 1
                    color: "#263845"
                    visible: formFlickable.contentHeight > formFlickable.height
                }

                Rectangle {
                    id: formVerticalScrollBar
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: page.scrollbarWidth
                    radius: page.scrollbarWidth / 2
                    color: "#0a1823"
                    border.color: "#223542"
                    visible: formFlickable.contentHeight > formFlickable.height

                    readonly property real scrollableHeight: Math.max(1, formFlickable.contentHeight - formFlickable.height)
                    readonly property real thumbHeight: Math.min(height, Math.max(42, height * formFlickable.visibleArea.heightRatio))
                    readonly property real thumbTravel: Math.max(0, height - thumbHeight)

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor

                        onPressed: function(mouse) {
                            var targetRatio = (mouse.y - formVerticalScrollBar.thumbHeight / 2)
                                / Math.max(1, formVerticalScrollBar.thumbTravel)
                            formFlickable.contentY = Math.max(0, Math.min(
                                formVerticalScrollBar.scrollableHeight,
                                targetRatio * formVerticalScrollBar.scrollableHeight))
                        }
                    }

                    Rectangle {
                        id: formScrollThumb
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: page.scrollbarWidth - 3
                        height: formVerticalScrollBar.thumbHeight
                        y: formVerticalScrollBar.thumbTravel * formFlickable.contentY
                            / formVerticalScrollBar.scrollableHeight
                        radius: width / 2
                        color: "#a8b0b6"

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor

                            property real pressY: 0
                            property real pressContentY: 0

                            onPressed: function(mouse) {
                                var point = mapToItem(formVerticalScrollBar, mouse.x, mouse.y)
                                pressY = point.y
                                pressContentY = formFlickable.contentY
                            }

                            onPositionChanged: function(mouse) {
                                if (pressed) {
                                    var point = mapToItem(formVerticalScrollBar, mouse.x, mouse.y)
                                    var delta = point.y - pressY
                                    var ratio = formVerticalScrollBar.scrollableHeight
                                        / Math.max(1, formVerticalScrollBar.thumbTravel)
                                    formFlickable.contentY = Math.max(0, Math.min(
                                        formVerticalScrollBar.scrollableHeight,
                                        pressContentY + delta * ratio))
                                }
                            }
                        }
                    }
                }
            }

            Button {
                Layout.preferredWidth: 88
                Layout.preferredHeight: 40
                enabled: !page.admissionLocked
                text: "Discard"
                onClicked: page.resetForm()
            }
        }
    }
}

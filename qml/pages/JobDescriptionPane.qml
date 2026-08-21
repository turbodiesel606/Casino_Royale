import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "../components"

Item {
    id: page
    z: 20

    property color textColor: "#eef3f8"
    property color mutedColor: "#a8b5c2"
    property color panelLineColor: "#223542"
    property color blueColor: "#1687ff"
    property var selectedApplication: ({})

    property bool editMode: false
    property bool saveInProgress: false
    property url replacementCvUrl: ""
    property string replacementCvName: ""
    property var fieldErrors: ({})
    property string saveError: ""
    property string baselineSnapshot: ""
    property double submittedOperationId: 0
    property string editingApplicationId: ""

    readonly property string selectedApplicationId: selectedApplication.id || ""
    readonly property string viewState: saveInProgress
        ? "saving"
        : (editMode ? "editing" : "read-only")
    readonly property bool hasUnsavedChanges: editMode
        && currentSnapshot() !== baselineSnapshot
    readonly property bool canSave: editMode
        && hasUnsavedChanges
        && editingApplicationId.length > 0
        && !saveInProgress
    readonly property string previewTitle: editMode
        ? jobTitleField.text
        : valueOrEmpty(selectedApplication.jobTitle)
    readonly property string previewCompany: editMode
        ? companyField.text
        : valueOrEmpty(selectedApplication.companyName)
    readonly property string previewWorkFormat: editMode
        ? workFormatField.text
        : valueOrEmpty(selectedApplication.workFormat)
    readonly property string previewSalary: editMode
        ? salaryField.text
        : valueOrEmpty(selectedApplication.salary)
    readonly property string previewStatus: editMode
        ? statusField.text
        : valueOrEmpty(selectedApplication.statusLabel || selectedApplication.status)
    readonly property string previewDate: editMode
        ? appliedDateField.text
        : valueOrEmpty(selectedApplication.appliedDate)
    readonly property string previewNextStep: editMode
        ? nextStepField.text
        : valueOrEmpty(selectedApplication.nextStep)
    readonly property string previewCvName: replacementCvName.length > 0
        ? replacementCvName
        : valueOrEmpty(selectedApplication.cvFileName)
    readonly property var previewTechStack: editMode
        ? technologiesFromText(techStackField.text)
        : (selectedApplication.techStack || [])

    signal applicationsRequested()
    signal updateSucceeded()
    signal updateFailed()

    function valueOrEmpty(value) {
        return value === undefined || value === null ? "" : String(value)
    }

    function technologiesFromText(text) {
        const values = String(text).split(",")
        const result = []
        for (let index = 0; index < values.length; ++index) {
            const value = values[index].trim()
            if (value.length > 0)
                result.push(value)
        }
        return result
    }

    function fileNameFromUrl(url) {
        const value = String(url)
        const separator = value.lastIndexOf("/")
        return decodeURIComponent(separator >= 0 ? value.substring(separator + 1) : value)
    }

    function errorFor(fieldName) {
        return fieldErrors && fieldErrors[fieldName]
            ? String(fieldErrors[fieldName])
            : ""
    }

    function errorSummary() {
        const messages = []
        if (saveError.length > 0)
            messages.push(saveError)
        if (fieldErrors) {
            for (const key in fieldErrors) {
                const value = String(fieldErrors[key])
                if (value.length > 0 && messages.indexOf(value) < 0)
                    messages.push(value)
            }
        }
        return messages.join(" ")
    }

    function currentSnapshot() {
        return JSON.stringify([
            jobTitleField.text,
            jobUrlField.text,
            companyField.text,
            workFormatField.text,
            cityField.text,
            salaryField.text,
            statusField.text,
            appliedDateField.text,
            nextStepField.text,
            descriptionField.text,
            requirementsField.text,
            techStackField.text,
            notesField.text,
            String(replacementCvUrl)
        ])
    }

    function draftFormValues() {
        return {
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
        }
    }

    function loadSavedState(exitEditMode) {
        jobTitleField.text = valueOrEmpty(selectedApplication.jobTitle)
        jobUrlField.text = valueOrEmpty(selectedApplication.jobUrl)
        companyField.text = valueOrEmpty(selectedApplication.companyName)
        workFormatField.text = valueOrEmpty(selectedApplication.workFormat)
        cityField.text = valueOrEmpty(selectedApplication.city)
        salaryField.text = valueOrEmpty(selectedApplication.salary)
        statusField.text = valueOrEmpty(
            selectedApplication.statusLabel || selectedApplication.status)
        appliedDateField.text = valueOrEmpty(selectedApplication.appliedDate)
        nextStepField.text = valueOrEmpty(selectedApplication.nextStep)
        descriptionField.text = valueOrEmpty(selectedApplication.description)
        requirementsField.text = valueOrEmpty(selectedApplication.requirements)
        techStackField.text = (selectedApplication.techStack || []).join(", ")
        notesField.text = valueOrEmpty(selectedApplication.notes)
        replacementCvUrl = ""
        replacementCvName = ""
        fieldErrors = ({})
        saveError = ""
        submittedOperationId = 0
        saveInProgress = false
        baselineSnapshot = currentSnapshot()
        if (exitEditMode) {
            editMode = false
            editingApplicationId = ""
        }
    }

    function beginEdit() {
        if (selectedApplicationId.length === 0 || saveInProgress)
            return
        const applicationId = selectedApplicationId
        loadSavedState(false)
        editingApplicationId = applicationId
        editMode = true
        baselineSnapshot = currentSnapshot()
        jobTitleField.forceActiveFocus()
    }

    function discardEdits() {
        if (saveInProgress)
            return
        loadSavedState(true)
    }

    function exitCleanEditMode() {
        if (editMode && !hasUnsavedChanges && !saveInProgress)
            loadSavedState(true)
    }

    function submitUpdate() {
        if (!editMode || !hasUnsavedChanges || saveInProgress
                || editingApplicationId.length === 0) {
            return
        }
        fieldErrors = ({})
        saveError = ""
        submittedOperationId = 0
        saveInProgress = true
        jobApplicationsController.updateApplication(
            editingApplicationId,
            draftFormValues(),
            replacementCvUrl)
    }

    function requestExplicitSave() {
        if (canSave)
            applyChangesConfirmation.open()
    }

    function statusAccent(status) {
        const normalized = String(status).toLowerCase()
        if (normalized === "interview")
            return "#ffbd21"
        if (normalized === "offer")
            return "#38c86b"
        if (normalized === "rejected")
            return "#ff4b49"
        if (normalized === "test task")
            return "#16c5dd"
        return "#c2c7cb"
    }

    onSelectedApplicationChanged: {
        if (!editMode) {
            Qt.callLater(function() {
                if (!page.editMode)
                    page.loadSavedState(false)
            })
        }
    }

    Component.onCompleted: loadSavedState(false)

    Connections {
        target: jobApplicationsController

        function onApplicationUpdateQueued(operationId, applicationId) {
            if (applicationId !== page.editingApplicationId)
                return
            page.submittedOperationId = operationId
            page.saveInProgress = true
        }

        function onApplicationUpdateRejected(operationId, applicationId, errors, message) {
            if (applicationId !== page.editingApplicationId)
                return
            page.submittedOperationId = 0
            page.saveInProgress = false
            page.fieldErrors = errors || ({})
            page.saveError = message || "The changes could not be saved."
            page.updateFailed()
        }

        function onApplicationUpdateCompleted(operationId, applicationId, jobTitle,
                                              success, errors, message) {
            if (applicationId !== page.editingApplicationId) {
                return
            }
            if (page.submittedOperationId !== 0
                    && operationId !== page.submittedOperationId) {
                return
            }
            page.submittedOperationId = 0
            page.saveInProgress = false
            if (success) {
                page.loadSavedState(true)
                page.updateSucceeded()
            } else {
                page.fieldErrors = errors || ({})
                page.saveError = message || "The changes could not be saved."
                page.updateFailed()
            }
        }
    }

    component FieldLabel: Text {
        color: page.textColor
        font.pixelSize: 13
        font.weight: Font.Medium
    }

    component FormField: TextField {
        id: control
        property string errorText: ""

        readOnly: !page.editMode || page.saveInProgress
        selectByMouse: true
        color: page.textColor
        placeholderTextColor: "#7f93a5"
        font.pixelSize: 15
        leftPadding: 12
        rightPadding: 12
        verticalAlignment: TextInput.AlignVCenter
        background: Rectangle {
            color: control.readOnly ? "#0a1822" : "#081923"
            border.color: control.errorText.length > 0 ? "#ff4b49" : "#263a48"
            radius: 5
        }
    }

    component FormArea: TextArea {
        id: control
        property string errorText: ""

        readOnly: !page.editMode || page.saveInProgress
        selectByMouse: true
        color: page.textColor
        placeholderTextColor: "#7f93a5"
        font.pixelSize: 14
        padding: 12
        wrapMode: TextEdit.WordWrap
        background: Rectangle {
            color: control.readOnly ? "#0a1822" : "#081923"
            border.color: control.errorText.length > 0 ? "#ff4b49" : "#263a48"
            radius: 5
        }
    }

    component TagChip: Rectangle {
        required property string label

        width: tagText.implicitWidth + 22
        height: 24
        radius: 7
        color: "#0b3d72"

        Text {
            id: tagText
            anchors.centerIn: parent
            text: parent.label
            color: "#dcefff"
            font.pixelSize: 13
        }
    }

    FileDialog {
        id: replacementCvDialog
        title: "Select replacement CV"
        fileMode: FileDialog.OpenFile
        nameFilters: ["CV documents (*.pdf *.doc *.docx)"]
        onAccepted: {
            page.replacementCvUrl = selectedFile
            page.replacementCvName = page.fileNameFromUrl(selectedFile)
            page.fieldErrors = ({})
            page.saveError = ""
        }
    }

    Dialog {
        id: applyChangesConfirmation
        anchors.centerIn: parent
        width: Math.min(560, page.width - 48)
        modal: true
        focus: true
        closePolicy: Popup.NoAutoClose
        title: "Confirm changes"

        contentItem: Text {
            text: "Are you sure you want to apply the changes?"
            color: page.textColor
            font.pixelSize: 15
            wrapMode: Text.Wrap
        }

        footer: DialogButtonBox {
            Button {
                text: "Cancel"
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                onClicked: applyChangesConfirmation.reject()
            }
            Button {
                text: "Apply Changes"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: {
                    applyChangesConfirmation.accept()
                    page.submitUpdate()
                }
            }
        }

        background: Rectangle {
            color: "#102330"
            border.color: "#2e4657"
            border.width: 1
            radius: 8
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#07131d"
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    spacing: 8

                    Button {
                        Layout.preferredWidth: 165
                        Layout.preferredHeight: 36
                        text: "Job Applications"
                        onClicked: page.applicationsRequested()
                        contentItem: Text {
                            text: parent.text
                            color: page.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 14
                        }
                        background: Rectangle {
                            color: "#0b1b27"
                            border.color: page.panelLineColor
                            radius: 5
                        }
                    }

                    Button {
                        Layout.preferredWidth: 165
                        Layout.preferredHeight: 36
                        text: "Job Description"
                        contentItem: Text {
                            text: parent.text
                            color: page.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }
                        background: Rectangle {
                            color: "#0b1b27"
                            border.color: page.blueColor
                            radius: 5
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                Panel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 14
                        contentWidth: availableWidth

                        ColumnLayout {
                            width: parent.width
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38

                                Text {
                                    text: page.saveInProgress
                                        ? "Saving Job Description..."
                                        : "Job Description"
                                    color: page.textColor
                                    font.bold: true
                                    font.pixelSize: 19
                                }

                                Item { Layout.fillWidth: true }

                                Button {
                                    Layout.preferredWidth: 112
                                    Layout.preferredHeight: 34
                                    visible: !page.editMode
                                    enabled: page.selectedApplicationId.length > 0
                                    text: "Edit"
                                    onClicked: page.beginEdit()
                                    contentItem: Text {
                                        text: parent.text
                                        color: "white"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                    }
                                    background: Rectangle {
                                        color: parent.enabled ? "#1479ee" : "#31506d"
                                        radius: 5
                                    }
                                }
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: width >= 760 ? 2 : 1
                                columnSpacing: 18
                                rowSpacing: 8

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Job Title" }
                                    FormField {
                                        id: jobTitleField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        errorText: page.errorFor("jobTitle")
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Job URL" }
                                    FormField {
                                        id: jobUrlField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        errorText: page.errorFor("jobUrl")
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Company" }
                                    FormField {
                                        id: companyField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        errorText: page.errorFor("companyName")
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Work Format" }
                                    FormField {
                                        id: workFormatField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        placeholderText: "Remote, Hybrid, or On-site"
                                        errorText: page.errorFor("workFormat")
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "City" }
                                    FormField {
                                        id: cityField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        errorText: page.errorFor("city")
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Salary" }
                                    FormField {
                                        id: salaryField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        errorText: page.errorFor("salary")
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Status" }
                                    FormField {
                                        id: statusField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        placeholderText: "Applied, Interview, Offer, Test Task, or Rejected"
                                        errorText: page.errorFor("status")
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Application Date (yyyy-MM-dd)" }
                                    FormField {
                                        id: appliedDateField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        errorText: page.errorFor("appliedDate")
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.columnSpan: parent.columns
                                    spacing: 5
                                    FieldLabel { text: "Next Step" }
                                    FormField {
                                        id: nextStepField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        errorText: page.errorFor("nextStep")
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 5
                                FieldLabel { text: "CV used" }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 48
                                    radius: 5
                                    color: "#081923"
                                    border.color: page.errorFor("cv").length > 0
                                        ? "#ff4b49"
                                        : "#263a48"

                                    Rectangle {
                                        x: 18
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 30
                                        height: 30
                                        radius: 4
                                        color: "#2588ff"
                                        Text {
                                            anchors.centerIn: parent
                                            text: "CV"
                                            color: "white"
                                            font.pixelSize: 9
                                            font.bold: true
                                        }
                                    }

                                    Text {
                                        x: 60
                                        anchors.right: changeCvButton.left
                                        anchors.rightMargin: 14
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: page.previewCvName
                                        color: page.textColor
                                        font.pixelSize: 15
                                        elide: Text.ElideMiddle
                                    }

                                    Button {
                                        id: changeCvButton
                                        anchors.right: parent.right
                                        anchors.rightMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 112
                                        height: 34
                                        visible: page.editMode
                                        enabled: !page.saveInProgress
                                        text: "Change CV"
                                        onClicked: replacementCvDialog.open()
                                        contentItem: Text {
                                            text: parent.text
                                            color: page.textColor
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            font.pixelSize: 13
                                        }
                                        background: Rectangle {
                                            color: "transparent"
                                            border.color: "#40576a"
                                            radius: 5
                                        }
                                    }
                                }
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: width >= 760 ? 2 : 1
                                columnSpacing: 18
                                rowSpacing: 8

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Description" }
                                    FormArea {
                                        id: descriptionField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 92
                                        errorText: page.errorFor("description")
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    FieldLabel { text: "Requirements" }
                                    FormArea {
                                        id: requirementsField
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 92
                                        errorText: page.errorFor("requirements")
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 5
                                FieldLabel { text: "Tech Stack" }

                                FormField {
                                    id: techStackField
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 38
                                    visible: page.editMode
                                    placeholderText: "Qt, C++, CMake"
                                    errorText: page.errorFor("techStack")
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: Math.max(38, childrenRect.height)
                                    visible: !page.editMode
                                    spacing: 8

                                    Repeater {
                                        model: page.previewTechStack
                                        delegate: TagChip {
                                            required property string modelData
                                            label: modelData
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 5
                                FieldLabel { text: "Notes" }
                                FormArea {
                                    id: notesField
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 72
                                    errorText: page.errorFor("notes")
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: page.errorSummary().length > 0
                                text: page.errorSummary()
                                color: "#ff6b69"
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 42
                                visible: page.editMode

                                Button {
                                    Layout.preferredWidth: 86
                                    Layout.preferredHeight: 38
                                    enabled: !page.saveInProgress
                                    text: "Discard"
                                    onClicked: page.discardEdits()
                                    contentItem: Text {
                                        text: parent.text
                                        color: page.textColor
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.pixelSize: 13
                                    }
                                    background: Rectangle {
                                        color: "#0b1b27"
                                        border.color: page.panelLineColor
                                        radius: 5
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                Button {
                                    Layout.preferredWidth: 132
                                    Layout.preferredHeight: 38
                                    enabled: page.canSave
                                    text: page.saveInProgress ? "Saving..." : "Save Changes"
                                    onClicked: page.requestExplicitSave()
                                    contentItem: Text {
                                        text: parent.text
                                        color: "white"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.pixelSize: 13
                                    }
                                    background: Rectangle {
                                        color: parent.enabled ? "#1479ee" : "#31506d"
                                        radius: 5
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 400
            Layout.fillHeight: true
            visible: page.width >= 1120
            radius: 0

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14

                Text {
                    Layout.fillWidth: true
                    text: "Live Preview"
                    color: page.textColor
                    font.bold: true
                    font.pixelSize: 18
                }

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 410

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14

                            Rectangle {
                                Layout.preferredWidth: 64
                                Layout.preferredHeight: 64
                                radius: 7
                                color: page.selectedApplication.companyAccent || "#146ce0"
                                Text {
                                    anchors.centerIn: parent
                                    text: page.previewCompany.substring(0, 2).toUpperCase()
                                    color: "white"
                                    font.pixelSize: 16
                                    font.bold: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 5
                                Text {
                                    Layout.fillWidth: true
                                    text: page.previewTitle
                                    color: page.textColor
                                    font.pixelSize: 20
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: page.previewCompany
                                    color: page.mutedColor
                                    font.pixelSize: 15
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        Repeater {
                            model: [
                                ["CV used", page.previewCvName, "link"],
                                ["Status", page.previewStatus, "status"],
                                ["Applied", page.previewDate, "text"],
                                ["Salary", page.previewSalary, "text"],
                                ["Format", page.previewWorkFormat, "text"],
                                ["Next step", page.previewNextStep, "text"]
                            ]

                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                Text {
                                    Layout.preferredWidth: 104
                                    text: modelData[0]
                                    color: page.mutedColor
                                    font.pixelSize: 14
                                }
                                StatusChip {
                                    visible: modelData[2] === "status"
                                    label: modelData[1]
                                    accent: page.statusAccent(modelData[1])
                                }
                                Text {
                                    Layout.fillWidth: true
                                    visible: modelData[2] !== "status"
                                    text: modelData[1]
                                    color: modelData[2] === "link"
                                        ? page.blueColor
                                        : page.textColor
                                    font.pixelSize: 14
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }

                Panel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 118

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10
                        Text {
                            text: "Tech Stack"
                            color: page.mutedColor
                            font.pixelSize: 14
                        }
                        Flow {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 8
                            Repeater {
                                model: page.previewTechStack
                                delegate: TagChip {
                                    required property string modelData
                                    label: modelData
                                }
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"
import "pages"

ApplicationWindow {
    id: window
    width: 1660
    height: 948
    minimumWidth: 1180
    minimumHeight: 720
    visible: true
    title: "JobTracker"
    color: "#07131d"

    property int currentPage: 0
    property bool jobFormVisible: false

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            Layout.fillHeight: true
            Layout.preferredWidth: 242
            currentIndex: window.currentPage
            onNavigate: index => {
                window.currentPage = index
                window.jobFormVisible = false
            }
            onAddJob: window.jobFormVisible = true
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.jobFormVisible ? 5 : window.currentPage

            DashboardPage { onAddJobRequested: window.jobFormVisible = true }
            JobsPage { onAddJobRequested: window.jobFormVisible = true }
            CvLibraryPage { }
            CompaniesPage { }
            ContactsPage { }
            JobFormPage {
                onCancelRequested: window.jobFormVisible = false
                onSaveRequested: window.jobFormVisible = false
            }
        }
    }
}

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
    property int previousPage: 0
    property bool jobFormVisible: false

    function openJobForm() {
        window.previousPage = window.currentPage >= 0 ? window.currentPage : 0
        window.currentPage = -1
        window.jobFormVisible = true
    }

    function closeJobForm() {
        window.jobFormVisible = false
        window.currentPage = window.previousPage
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            Layout.fillHeight: true
            Layout.preferredWidth: 242
            currentIndex: window.jobFormVisible ? -1 : window.currentPage
            onNavigate: index => {
                window.currentPage = index
                window.jobFormVisible = false
            }
            onAddJob: window.openJobForm()
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.jobFormVisible ? 5 : Math.max(window.currentPage, 0)

            DashboardPage { onAddJobRequested: window.openJobForm() }
            JobsPage { onAddJobRequested: window.openJobForm() }
            CvLibraryPage { }
            CompaniesPage { }
            ContactsPage { }
            JobFormPage {
                onCancelRequested: window.closeJobForm()
                onSaveRequested: window.closeJobForm()
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FM_UI


ApplicationWindow {
    id: root
    visible: true
    width: 1200
    height: 900
    minimumWidth: 900
    minimumHeight: 700
    title: "Football Manager"
    color: "#f5f5f5"

     property string currentView: gameFacade.hasStartedGame() ? "dashboard" : "newGame"
    property string headerTeamName: gameFacade.hasStartedGame() ? (gameFacade.getSelectedTeamName() || "No Team") : "Football Manager"
    property string headerDateText: gameFacade.getCurrentDateText() || ""

     function refreshHeader() {
        headerTeamName = gameFacade.hasStartedGame() ? (gameFacade.getSelectedTeamName() || "No Team") : "Football Manager"
        headerDateText = gameFacade.getCurrentDateText() || ""
    }

    function goTo(viewName) {
        currentView = viewName
    }

    function refreshActiveView() {
        if (viewLoader.item && viewLoader.item.refreshData) {
            viewLoader.item.refreshData()
        }
    }

    Component.onCompleted: refreshHeader()

    header: ToolBar {
        visible: root.currentView !== "newGame"
        contentHeight: 52

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            Label {
                text: root.headerTeamName
                font.pixelSize: 20
                font.bold: true
                color: "#202020"
            }

            Item { Layout.fillWidth: true }

            Label {
                visible: gameFacade.hasStartedGame()
                text: root.headerDateText
                color: "#444444"
                font.pixelSize: 14
            }
        }
    }

    Loader {
        id: viewLoader
        anchors.fill: parent
        anchors.margins: root.currentView === "newGame" ? 0 : 16
        sourceComponent: root.currentView === "newGame"
                         ? newGameComponent
                         : root.currentView === "standings"
                           ? standingsComponent
                           : root.currentView === "team"
                             ? teamComponent
                             : dashboardComponent
        onLoaded: {
            root.refreshHeader()
            root.refreshActiveView()
        }
    }

    Connections {
        target: gameFacade

        function onGameStateChanged() {
            if (gameFacade.hasStartedGame() && root.currentView === "newGame") {
                root.currentView = "dashboard"
            }
            root.refreshHeader()
            root.refreshActiveView()
        }
    }

    Component {
        id: newGameComponent

        NewGameView {
            onGameStarted: root.goTo("dashboard")
        }
    }

    Component {
        id: dashboardComponent

        DashboardView {
            onOpenStandingsRequested: root.goTo("standings")
            onOpenTeamRequested: root.goTo("team")
        }
    }

    Component {
        id: standingsComponent

        StandingsView {
            onBackRequested: root.goTo("dashboard")
        }
    }

    Component {
        id: teamComponent
        TeamView {
            onBackRequested: root.goTo("dashboard")
        }
    }
}
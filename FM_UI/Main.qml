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

    property string currentView: gameFacade.hasStartedGame() ? "dashboard" : "home"
    property string headerTeamName: gameFacade.hasStartedGame() ? (gameFacade.getSelectedTeamName() || "No Team") : "Football Manager"
    property string headerDateText: gameFacade.getCurrentDateText() || ""
    property bool hasActiveInteraction: gameFacade.hasActiveInteraction()
    property string activeInteractionKind: gameFacade.getActiveInteractionKind() || ""
    property bool simulationPaused: gameFacade.isTimePaused()

    function refreshHeader() {
        headerTeamName = gameFacade.hasStartedGame() ? (gameFacade.getSelectedTeamName() || "No Team") : "Football Manager"
        headerDateText = gameFacade.getCurrentDateText() || ""
    }

    function refreshInteractionState() {
        hasActiveInteraction = gameFacade.hasActiveInteraction()
        activeInteractionKind = gameFacade.getActiveInteractionKind() || ""
        simulationPaused = gameFacade.isTimePaused()
    }

    function goTo(viewName) {
        currentView = viewName
    }

    function refreshActiveView() {
        if (viewLoader.item && viewLoader.item.refreshData) {
            viewLoader.item.refreshData()
        }
    }

    Component.onCompleted: {
        refreshHeader()
        refreshInteractionState()
    }

    header: ToolBar {
        visible: root.currentView !== "home" && root.currentView !== "teamSelection"
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
       anchors.margins: (root.currentView === "dashboard" || root.currentView === "standings" || root.currentView === "team") ? 16 : 0
        sourceComponent: root.currentView === "home"
                         ? homeComponent
                         : root.currentView === "teamSelection"
                           ? newGameComponent
                           : root.currentView === "standings"
                             ? standingsComponent
                             : root.currentView === "team"
                               ? teamComponent
                               : dashboardComponent
        onLoaded: {
            root.refreshHeader()
            root.refreshActiveView()
            root.refreshInteractionState()
        }
    }

    Connections {
        target: gameFacade

        function onGameStateChanged() {
           if (gameFacade.hasStartedGame() && root.currentView === "teamSelection") {
                root.currentView = "dashboard"
            }
            root.refreshHeader()
            root.refreshActiveView()
            root.refreshInteractionState()
        }
    }

        Timer {
        id: simulationTimer
        interval: 300
        repeat: true
        running: gameFacade.hasStartedGame() && !root.simulationPaused && !root.hasActiveInteraction
        onTriggered: gameFacade.advanceOneDay()
    }

    Component {
        id: homeComponent

        HomeView {
            onNewGameRequested: {
                gameFacade.clearLastError()
                root.goTo("teamSelection")
            }
            onQuitRequested: Qt.quit()
        }
    }

    Component {
        id: newGameComponent

        NewGameView {
        onBackRequested: {
                gameFacade.clearLastError()
                root.goTo("home")
            }
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


    PostMatchDialog {
        id: postMatchDialog
        anchors.fill: parent
        visible: root.hasActiveInteraction && root.activeInteractionKind === "post_match"
        interactionData: gameFacade.getActivePostMatchInteraction()
        onContinueRequested: gameFacade.resolveActiveInteraction()
    }

    TransferOfferDialog {
        id: transferOfferDialog
        anchors.fill: parent
        visible: root.hasActiveInteraction && root.activeInteractionKind === "transfer_offer"
        interactionData: gameFacade.getActiveTransferOfferInteraction()
        onAcceptRequested: gameFacade.acceptActiveTransferOffer()
        onRejectRequested: gameFacade.rejectActiveTransferOffer()
        onLaterRequested: gameFacade.deferActiveTransferOffer()
    }
}
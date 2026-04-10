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
    property bool gameStarted: gameFacade.hasStartedGame()
    property string headerTeamName: gameStarted ? (gameFacade.getSelectedTeamName() || "No Team") : "Football Manager"
    property string headerDateText: gameFacade.getCurrentDateText() || ""
    property bool hasActiveInteraction: gameFacade.hasActiveInteraction()
    property string activeInteractionKind: gameFacade.getActiveInteractionKind() || ""
    property bool simulationPaused: gameFacade.isTimePaused()
    property var activePostMatchData: ({})
    property var activeTransferOfferData: ({})

    function refreshHeader() {
        gameStarted = gameFacade.hasStartedGame()
        headerTeamName = gameStarted ? (gameFacade.getSelectedTeamName() || "No Team") : "Football Manager"
        headerDateText = gameFacade.getCurrentDateText() || ""
    }

    function refreshInteractionState() {
        hasActiveInteraction = gameFacade.hasActiveInteraction()
        activeInteractionKind = gameFacade.getActiveInteractionKind() || ""
        simulationPaused = gameFacade.isTimePaused()

        activePostMatchData = (hasActiveInteraction && activeInteractionKind === "post_match")
                              ? (gameFacade.getActivePostMatchInteraction() || {})
                              : ({})
        activeTransferOfferData = (hasActiveInteraction && activeInteractionKind === "transfer_offer")
                                  ? (gameFacade.getActiveTransferOfferInteraction() || {})
                                  : ({})
    }

    function refreshActiveView() {
        if (viewLoader.item && viewLoader.item.refreshData) {
            viewLoader.item.refreshData()
        }
    }

    function refreshUiState() {
        refreshInteractionState()
        refreshHeader()
        refreshActiveView()
    }

    function goTo(viewName) {
        currentView = viewName
    }

    Component.onCompleted: {
        refreshUiState()
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
                visible: root.gameStarted
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
            root.refreshUiState()
        }
    }

    Connections {
        target: gameFacade

        function onGameStateChanged() {
            if (gameFacade.hasStartedGame() && root.currentView === "teamSelection") {
                root.currentView = "dashboard"
            }
            root.refreshUiState()
        }
    }

    Timer {
        id: simulationTimer
        interval: 300
        repeat: true
        running: root.gameStarted && !root.simulationPaused && !root.hasActiveInteraction
        onTriggered: {
            gameFacade.advanceOneDay()
            root.refreshUiState()
        }
    }

    Component {
        id: homeComponent

        HomeView {
            onNewGameRequested: {
                gameFacade.clearLastError()
                root.goTo("teamSelection")
                root.refreshUiState()
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
                root.refreshUiState()
            }
            onGameStarted: {
                root.goTo("dashboard")
                root.refreshUiState()
            }
        }
    }

    Component {
        id: dashboardComponent

        DashboardView {
            onOpenStandingsRequested: {
                root.goTo("standings")
                root.refreshUiState()
            }
            onOpenTeamRequested: {
                root.goTo("team")
                root.refreshUiState()
            }
        }
    }

    Component {
        id: standingsComponent

        StandingsView {
            onBackRequested: {
                root.goTo("dashboard")
                root.refreshUiState()
            }
        }
    }

    Component {
        id: teamComponent
        TeamView {
            onBackRequested: {
                root.goTo("dashboard")
                root.refreshUiState()
            }
        }
    }

    PostMatchDialog {
        id: postMatchDialog
        anchors.fill: parent
        visible: root.hasActiveInteraction && root.activeInteractionKind === "post_match"
        interactionData: root.activePostMatchData
        onContinueRequested: {
            gameFacade.resolveActiveInteraction()
            root.refreshUiState()
        }
    }

    TransferOfferDialog {
        id: transferOfferDialog
        anchors.fill: parent
        visible: root.hasActiveInteraction && root.activeInteractionKind === "transfer_offer"
        interactionData: root.activeTransferOfferData
        onAcceptRequested: {
            gameFacade.acceptActiveTransferOffer()
            root.refreshUiState()
        }
        onRejectRequested: {
            gameFacade.rejectActiveTransferOffer()
            root.refreshUiState()
        }
        onLaterRequested: {
            gameFacade.deferActiveTransferOffer()
            root.refreshUiState()
        }
    }
}

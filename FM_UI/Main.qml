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

    readonly property QtObject routes: QtObject {
        readonly property string home: "home"
        readonly property string teamSelection: "teamSelection"
        readonly property string dashboard: "dashboard"
        readonly property string standings: "standings"
        readonly property string team: "team"
        readonly property string transfers: "transfers"
    }

    readonly property QtObject interactionKinds: QtObject {
        readonly property string preMatch: "pre_match"
        readonly property string postMatch: "post_match"
        readonly property string transferOffer: "transfer_offer"
    }

    property string currentView: gameFacade.hasStartedGame() ? routes.dashboard : routes.home
    property bool gameStarted: gameFacade.hasStartedGame()
    property string headerTeamName: gameStarted ? (gameFacade.getSelectedTeamName() || "No Team") : "Football Manager"
    property string headerDateText: gameFacade.getCurrentDateText() || ""
    property bool hasActiveInteraction: gameFacade.hasActiveInteraction()
    property string activeInteractionKind: gameFacade.getActiveInteractionKind() || ""
    property var activePreMatchData: ({})
    property bool simulationPaused: gameFacade.isTimePaused()
    property var activePostMatchData: ({})
    property var activeTransferOfferData: ({})

    function componentForView(viewName) {
        if (viewName === routes.home) {
            return homeComponent
        }
        if (viewName === routes.teamSelection) {
            return newGameComponent
        }
        if (viewName === routes.standings) {
            return standingsComponent
        }
        if (viewName === routes.team) {
            return teamComponent
        }
        if (viewName === routes.transfers) {
            return transfersComponent
        }
        return dashboardComponent
    }

    function viewUsesPageMargins(viewName) {
        return viewName === routes.dashboard
               || viewName === routes.standings
               || viewName === routes.team
               || viewName === routes.transfers
    }

    function refreshHeader() {
        gameStarted = gameFacade.hasStartedGame()
        headerTeamName = gameStarted ? (gameFacade.getSelectedTeamName() || "No Team") : "Football Manager"
        headerDateText = gameFacade.getCurrentDateText() || ""
    }

    function refreshInteractionState() {
        hasActiveInteraction = gameFacade.hasActiveInteraction()
        activeInteractionKind = gameFacade.getActiveInteractionKind() || ""
        simulationPaused = gameFacade.isTimePaused()

        activePreMatchData = (hasActiveInteraction && activeInteractionKind === interactionKinds.preMatch)
                             ? (gameFacade.getActivePreMatchInteraction() || {})
                             : ({})

        activePostMatchData = (hasActiveInteraction && activeInteractionKind === interactionKinds.postMatch)
                              ? (gameFacade.getActivePostMatchInteraction() || {})
                              : ({})
        activeTransferOfferData = (hasActiveInteraction && activeInteractionKind === interactionKinds.transferOffer)
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

    function navigateTo(viewName) {
        goTo(viewName)
        refreshUiState()
    }

    function pauseSimulation() {
        gameFacade.pauseSimulation()
        refreshUiState()
    }

    function resumeSimulation() {
        gameFacade.resumeSimulation()
        refreshUiState()
    }

    function resolveActiveInteraction() {
        gameFacade.resolveActiveInteraction()
        refreshUiState()
    }

    function playActiveMatch() {
        gameFacade.playActiveMatch()
        refreshUiState()
    }

    function acceptActiveTransferOffer() {
        gameFacade.acceptActiveTransferOffer()
        refreshUiState()
    }

    function rejectActiveTransferOffer() {
        gameFacade.rejectActiveTransferOffer()
        refreshUiState()
    }

    function deferActiveTransferOffer() {
        gameFacade.deferActiveTransferOffer()
        refreshUiState()
    }

    Component.onCompleted: {
        refreshUiState()
    }

    header: ToolBar {
        visible: root.currentView !== root.routes.home && root.currentView !== root.routes.teamSelection
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
        anchors.margins: root.viewUsesPageMargins(root.currentView) ? 16 : 0
        sourceComponent: root.componentForView(root.currentView)
        onLoaded: {
            root.refreshUiState()
        }
    }

    Connections {
        target: gameFacade

        function onGameStateChanged() {
            if (gameFacade.hasStartedGame() && root.currentView === root.routes.teamSelection) {
                root.currentView = root.routes.dashboard
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
                root.navigateTo(root.routes.teamSelection)
            }
            onQuitRequested: Qt.quit()
        }
    }

    Component {
        id: newGameComponent

        NewGameView {
            onBackRequested: {
                gameFacade.clearLastError()
                root.navigateTo(root.routes.home)
            }
            onGameStarted: {
                root.navigateTo(root.routes.dashboard)
            }
        }
    }

    Component {
        id: dashboardComponent

        DashboardView {
            onOpenStandingsRequested: {
                root.navigateTo(root.routes.standings)
            }
            onOpenTeamRequested: {
                root.navigateTo(root.routes.team)
            }
            onOpenTransfersRequested: {
                root.navigateTo(root.routes.transfers)
            }
            onPauseRequested: {
                root.pauseSimulation()
            }
            onResumeRequested: {
                root.resumeSimulation()
            }
        }
    }

    Component {
        id: standingsComponent

        StandingsView {
            onBackRequested: {
                root.navigateTo(root.routes.dashboard)
            }
        }
    }

    Component {
        id: teamComponent
        TeamView {
            onBackRequested: {
                root.navigateTo(root.routes.dashboard)
            }
        }
    }

    Component {
        id: transfersComponent
        TransferOffersView {
            onBackRequested: {
                root.navigateTo(root.routes.dashboard)
            }
        }
    }

    PostMatchDialog {
        id: postMatchDialog
        anchors.fill: parent
        visible: root.hasActiveInteraction && root.activeInteractionKind === root.interactionKinds.postMatch
        interactionData: root.activePostMatchData
        onContinueRequested: {
            root.resolveActiveInteraction()
        }
    }

    PreMatchDialog {
        id: preMatchDialog
        anchors.fill: parent
        visible: root.hasActiveInteraction && root.activeInteractionKind === root.interactionKinds.preMatch
        interactionData: root.activePreMatchData
        onPlayMatchRequested: {
            root.playActiveMatch()
        }
    }

    TransferOfferDialog {
        id: transferOfferDialog
        anchors.fill: parent
        visible: root.hasActiveInteraction && root.activeInteractionKind === root.interactionKinds.transferOffer
        interactionData: root.activeTransferOfferData
        onAcceptRequested: {
            root.acceptActiveTransferOffer()
        }
        onRejectRequested: {
            root.rejectActiveTransferOffer()
        }
        onLaterRequested: {
            root.deferActiveTransferOffer()
        }
    }
}

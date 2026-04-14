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

    readonly property var shellState: gameFacade.shellState
    readonly property var interactionState: gameFacade.interactionState

    property string currentView: shellState.hasStartedGame ? routes.dashboard : routes.home

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

    function goTo(viewName) {
        currentView = viewName
    }

    function navigateTo(viewName) {
        goTo(viewName)
    }

    function pauseSimulation() {
        gameFacade.pauseSimulation()
    }

    function resumeSimulation() {
        gameFacade.resumeSimulation()
    }

    function resolveActiveInteraction() {
        gameFacade.resolveActiveInteraction()
    }

    function playActiveMatch() {
        gameFacade.playActiveMatch()
    }

    function acceptActiveTransferOffer() {
        gameFacade.acceptActiveTransferOffer()
    }

    function rejectActiveTransferOffer() {
        gameFacade.rejectActiveTransferOffer()
    }

    function deferActiveTransferOffer() {
        gameFacade.deferActiveTransferOffer()
    }

    function openMatchDetails(matchId) {
        matchDetailDialog.openForMatch(matchId)
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
                text: root.shellState.hasStartedGame
                      ? (root.shellState.selectedTeamName || "No Team")
                      : "Football Manager"
                font.pixelSize: 20
                font.bold: true
                color: "#202020"
            }

            Item { Layout.fillWidth: true }

            Label {
                visible: root.shellState.hasStartedGame
                text: root.shellState.currentDateText || ""
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
    }

    Timer {
        id: simulationTimer
        interval: 300
        repeat: true
        running: root.shellState.hasStartedGame
                 && !root.shellState.timePaused
                 && !root.interactionState.hasActiveInteraction
        onTriggered: {
            gameFacade.advanceOneDay()
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
            onOpenMatchDetailRequested: function(matchId) {
                root.openMatchDetails(matchId)
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
        visible: root.interactionState.hasActiveInteraction
                 && root.interactionState.kind === root.interactionKinds.postMatch
        interactionData: root.interactionState.postMatch
        onViewDetailsRequested: function(matchId) {
            root.openMatchDetails(matchId)
        }
        onContinueRequested: {
            root.resolveActiveInteraction()
        }
    }

    PreMatchDialog {
        id: preMatchDialog
        anchors.fill: parent
        visible: root.interactionState.hasActiveInteraction
                 && root.interactionState.kind === root.interactionKinds.preMatch
        interactionData: root.interactionState.preMatch
        onPlayMatchRequested: {
            root.playActiveMatch()
        }
    }

    TransferOfferDialog {
        id: transferOfferDialog
        anchors.fill: parent
        visible: root.interactionState.hasActiveInteraction
                 && root.interactionState.kind === root.interactionKinds.transferOffer
        interactionData: root.interactionState.transferOffer
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

    MatchDetailDialog {
        id: matchDetailDialog
        anchors.fill: parent
        visible: false
    }
}

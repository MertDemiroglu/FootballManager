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
    color: "#071016"

    readonly property QtObject routes: QtObject {
        readonly property string home: "home"
        readonly property string teamSelection: "teamSelection"
        readonly property string dashboard: "dashboard"
        readonly property string standings: "standings"
        readonly property string team: "team"
        readonly property string lineupEditor: "lineupEditor"
        readonly property string transfers: "transfers"
        readonly property string preMatch: "preMatch"
        readonly property string postMatch: "postMatch"
    }

    readonly property QtObject interactionKinds: QtObject {
        readonly property string preMatch: "pre_match"
        readonly property string postMatch: "post_match"
        readonly property string transferOffer: "transfer_offer"
    }

    readonly property var shellState: gameFacade.shellState
    readonly property var interactionState: gameFacade.interactionState

    property string currentView: shellState.hasStartedGame ? routes.dashboard : routes.home
    property string returnRouteAfterLineupEditor: ""
    property bool matchFlowTransitionInProgress: false
    property var lastValidPreMatchData: ({})

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
        if (viewName === routes.lineupEditor) {
            return lineupEditorComponent
        }
        if (viewName === routes.preMatch) {
            return preMatchComponent
        }
        if (viewName === routes.postMatch) {
            return postMatchComponent
        }
        return dashboardComponent
    }

    function viewUsesPageMargins(viewName) {
        return viewName === routes.standings
               || viewName === routes.team
               || viewName === routes.transfers
    }

    function goTo(viewName) {
        currentView = viewName
    }

    function navigateTo(viewName) {
        if (viewName === routes.preMatch) {
            cachePreMatchDataIfValid()
        }
        goTo(viewName)
    }

    function hasCachedPreMatchData() {
        return lastValidPreMatchData && lastValidPreMatchData.hasData === true
    }

    function snapshotPreMatchData(data) {
        return {
            hasData: data.hasData,
            matchId: data.matchId,
            dateText: data.dateText || "",
            matchweek: data.matchweek || 0,
            homeTeamId: data.homeTeamId || 0,
            awayTeamId: data.awayTeamId || 0,
            homeTeamName: data.homeTeamName || "",
            awayTeamName: data.awayTeamName || "",
            homePrimaryColor: data.homePrimaryColor || "#22c55e",
            homeSecondaryColor: data.homeSecondaryColor || "#0f172a",
            homeTextColor: data.homeTextColor || "#f8fafc",
            awayPrimaryColor: data.awayPrimaryColor || "#22c55e",
            awaySecondaryColor: data.awaySecondaryColor || "#0f172a",
            awayTextColor: data.awayTextColor || "#f8fafc",
            homeRecentForm: data.homeRecentForm || "",
            awayRecentForm: data.awayRecentForm || "",
            homeFormationText: data.homeFormationText || "",
            awayFormationText: data.awayFormationText || "",
            formationText: data.formationText || "",
            homeAverageOverallText: data.homeAverageOverallText || "--",
            awayAverageOverallText: data.awayAverageOverallText || "--",
            homeLineup: data.homeLineup || [],
            awayLineup: data.awayLineup || []
        }
    }

    function cachePreMatchDataIfValid() {
        if (interactionState.hasActiveInteraction
            && interactionState.kind === interactionKinds.preMatch
            && interactionState.preMatch
            && interactionState.preMatch.hasData) {
            lastValidPreMatchData = snapshotPreMatchData(interactionState.preMatch)
        }
    }

    function effectivePreMatchData() {
        if (matchFlowTransitionInProgress && hasCachedPreMatchData()) {
            return lastValidPreMatchData
        }
        return interactionState.preMatch
    }

    function finishMatchFlowTransitionOrFallback() {
        if (interactionState.hasActiveInteraction
            && interactionState.kind === interactionKinds.postMatch) {
            navigateTo(routes.postMatch)
            matchFlowTransitionInProgress = false
            return
        }

        navigateTo(routes.dashboard)
        matchFlowTransitionInProgress = false
    }

    function playActiveMatchAndNavigateToPostMatch() {
        cachePreMatchDataIfValid()
        matchFlowTransitionInProgress = true
        gameFacade.playActiveMatch()

        Qt.callLater(function() {
            if (interactionState.hasActiveInteraction
                && interactionState.kind === interactionKinds.postMatch) {
                navigateTo(routes.postMatch)
                matchFlowTransitionInProgress = false
                return
            }

            Qt.callLater(function() {
                root.finishMatchFlowTransitionOrFallback()
            })
        })
    }

    function openLineupEditor(returnRoute) {
        returnRouteAfterLineupEditor = returnRoute || routes.dashboard
        navigateTo(routes.lineupEditor)
    }

    function returnFromLineupEditor() {
        const targetRoute = returnRouteAfterLineupEditor
        returnRouteAfterLineupEditor = ""
        if (targetRoute === routes.preMatch) {
            gameFacade.applyEditableLineupToActivePreMatch()
            if (root.interactionState.hasActiveInteraction
                && root.interactionState.kind === root.interactionKinds.preMatch) {
                root.navigateTo(root.routes.preMatch)
            } else {
                root.navigateTo(root.routes.dashboard)
            }
            return
        }

        root.navigateTo(root.routes.dashboard)
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
        visible: root.currentView !== root.routes.home
                 && root.currentView !== root.routes.teamSelection
                 && root.currentView !== root.routes.lineupEditor
                 && root.currentView !== root.routes.dashboard
                 && root.currentView !== root.routes.preMatch
                 && root.currentView !== root.routes.postMatch
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
            onOpenLineupEditorRequested: {
                root.openLineupEditor(root.routes.dashboard)
            }
            onOpenPreMatchRequested: {
                root.navigateTo(root.routes.preMatch)
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
            onOpenLineupEditorRequested: {
                root.openLineupEditor(root.routes.dashboard)
            }
            onOpenMatchDetailRequested: function(matchId) {
                root.openMatchDetails(matchId)
            }
        }
    }

    Component {
        id: lineupEditorComponent
        LineupEditorScreen {
            onBackRequested: {
                root.returnFromLineupEditor()
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

    Connections {
        target: root.interactionState
        function onChanged() {
            root.cachePreMatchDataIfValid()
        }
    }

    Connections {
        target: root.interactionState.preMatch
        function onChanged() {
            root.cachePreMatchDataIfValid()
        }
    }

    Component {
        id: preMatchComponent

        PreMatchScreen {
            interactionData: root.effectivePreMatchData()
            suppressFallback: root.matchFlowTransitionInProgress
            selectedTeamName: root.shellState.selectedTeamName || "No Team"
            selectedTeamPrimaryColor: root.shellState.selectedTeamPrimaryColor || "#22c55e"
            selectedTeamSecondaryColor: root.shellState.selectedTeamSecondaryColor || "#0f172a"
            selectedTeamTextColor: root.shellState.selectedTeamTextColor || "#f8fafc"
            currentDateText: root.shellState.currentDateText || ""
            onBackRequested: {
                root.navigateTo(root.routes.dashboard)
            }
            onEditLineupRequested: {
                root.openLineupEditor(root.routes.preMatch)
            }
            onPlayMatchRequested: {
                root.playActiveMatchAndNavigateToPostMatch()
            }
        }
    }

    Component {
        id: postMatchComponent

        PostMatchScreen {
            interactionData: root.interactionState.postMatch
            selectedTeamName: root.shellState.selectedTeamName || "No Team"
            selectedTeamPrimaryColor: root.shellState.selectedTeamPrimaryColor || "#22c55e"
            selectedTeamSecondaryColor: root.shellState.selectedTeamSecondaryColor || "#0f172a"
            selectedTeamTextColor: root.shellState.selectedTeamTextColor || "#f8fafc"
            currentDateText: root.shellState.currentDateText || ""
            onBackRequested: {
                root.navigateTo(root.routes.dashboard)
            }
            onViewDetailsRequested: function(matchId) {
                root.openMatchDetails(matchId)
            }
            onContinueRequested: {
                root.resolveActiveInteraction()
                root.navigateTo(root.routes.dashboard)
            }
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

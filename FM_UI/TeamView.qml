import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    signal backRequested()

    property var seasonStats: ({})
    property var recentMatches: []
    property var upcomingMatches: []
    property var players: []
    property var selectedPlayerDetails: ({})
    property bool hasActiveGame: gameFacade.hasStartedGame()

    function refreshData() {
        hasActiveGame = gameFacade.hasStartedGame()
        seasonStats = gameFacade.getCurrentTeamSeasonStats()
        recentMatches = gameFacade.getCurrentTeamMatches()
        upcomingMatches = gameFacade.getCurrentTeamUpcomingMatches(5)
        players = gameFacade.getCurrentTeamPlayers()
    }

    function openPlayerDetails(playerId) {
     if (!root.hasActiveGame) {
            return
        }
        selectedPlayerDetails = gameFacade.getPlayerDetails(playerId)
        playerDialog.open()
    }

    function mapValue(mapObject, key, fallbackValue) {
        if (!mapObject || mapObject[key] === undefined || mapObject[key] === null) {
            return fallbackValue
        }
        return mapObject[key]
    }

    Component.onCompleted: refreshData()

    Connections {
        target: gameFacade

        function onGameStateChanged() {
            refreshData()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: gameFacade.getSelectedTeamName() || "Team"
                font.pixelSize: 28
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Back"
                onClicked: root.backRequested()
            }
        }

        Label {
            text: "Team overview, fixtures and squad information."
            color: "#666666"
        }

        Label {
            visible: !root.hasActiveGame
            text: "No active game. Start a new game to view team details."
            color: "#666666"
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.hasActiveGame
            clip: true

            ColumnLayout {
                width: root.width - 16
                spacing: 16

                Rectangle {
                    Layout.fillWidth: true
                    radius: 10
                    color: "white"
                    border.color: "#d8d8d8"

                    GridLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        columns: 4
                        columnSpacing: 16
                        rowSpacing: 10

                        Label { text: "Played"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "played", 0) }
                        Label { text: "Wins"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "wins", 0) }

                        Label { text: "Draws"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "draws", 0) }
                        Label { text: "Losses"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "losses", 0) }

                        Label { text: "Goals For"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "goalsFor", 0) }
                        Label { text: "Goals Against"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "goalsAgainst", 0) }

                        Label { text: "Goal Diff"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "goalDifference", 0) }
                        Label { text: "Points"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "points", 0) }

                        Label { text: "Clean Sheets"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "cleanSheets", 0) }
                        Label { text: "Failed To Score"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "failedToScore", 0) }

                        Label { text: "Home Played"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "homePlayed", 0) }
                        Label { text: "Away Played"; font.bold: true }
                        Label { text: root.mapValue(root.seasonStats, "awayPlayed", 0) }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 10
                    color: "white"
                    border.color: "#d8d8d8"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 10

                        Label {
                            text: "Recent Matches"
                            font.pixelSize: 20
                            font.bold: true
                        }

                        Repeater {
                            model: root.recentMatches

                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: 48
                                radius: 6
                                color: "#fafafa"
                                border.color: "#e2e2e2"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    Label { Layout.preferredWidth: 150; text: modelData.dateText || "-" }
                                    Label { Layout.fillWidth: true; text: modelData.opponentName || "-"; elide: Text.ElideRight }
                                    Label { Layout.preferredWidth: 80; text: modelData.isHome ? "Home" : "Away" }
                                    Label { Layout.preferredWidth: 90; text: (modelData.goalsFor || 0) + " - " + (modelData.goalsAgainst || 0) }
                                    Label { Layout.preferredWidth: 50; text: modelData.resultLetter || "-"; font.bold: true }
                                    Label { Layout.preferredWidth: 70; text: "MW " + (modelData.matchweek || 0) }
                                }
                            }
                        }

                        Label {
                            visible: root.recentMatches.length === 0
                            text: "No data"
                            color: "#666666"
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 10
                    color: "white"
                    border.color: "#d8d8d8"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 10

                        Label {
                            text: "Upcoming Matches"
                            font.pixelSize: 20
                            font.bold: true
                        }

                        Repeater {
                            model: root.upcomingMatches

                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: 48
                                radius: 6
                                color: "#fafafa"
                                border.color: "#e2e2e2"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    Label { Layout.preferredWidth: 150; text: modelData.dateText || "-" }
                                    Label { Layout.fillWidth: true; text: (modelData.homeTeamName || "") + " vs " + (modelData.awayTeamName || "") }
                                    Label { Layout.preferredWidth: 80; text: modelData.isHome ? "Home" : "Away" }
                                    Label { Layout.preferredWidth: 70; text: "MW " + (modelData.matchweek || 0) }
                                }
                            }
                        }

                        Label {
                            visible: root.upcomingMatches.length === 0
                            text: "No upcoming match"
                            color: "#666666"
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 10
                    color: "white"
                    border.color: "#d8d8d8"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 10

                        Label {
                            text: "Players"
                            font.pixelSize: 20
                            font.bold: true
                        }

                        Repeater {
                            model: root.players

                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: 52
                                radius: 6
                                color: "#fafafa"
                                border.color: "#e2e2e2"

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.openPlayerDetails(parent.modelData.playerId)
                                    cursorShape: Qt.PointingHandCursor
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    Label { Layout.fillWidth: true; text: modelData.name || "Unknown"; font.bold: true }
                                    Label { Layout.preferredWidth: 50; text: modelData.age || 0 }
                                    Label { Layout.preferredWidth: 100; text: modelData.position || "-" }
                                    Label { Layout.preferredWidth: 90; text: modelData.overallSummary || "-" }
                                    Label { Layout.preferredWidth: 130; text: "Apps " + (modelData.appearances || 0) + " · G " + (modelData.goals || 0) + " · A " + (modelData.assists || 0) }
                                }
                            }
                        }

                        Label {
                            visible: root.players.length === 0
                            text: "No data"
                            color: "#666666"
                        }
                    }
                }
            }
        }
    }

    PlayerDetailsDialog {
        id: playerDialog
        parent: Overlay.overlay
        playerDetails: root.selectedPlayerDetails
    }
}
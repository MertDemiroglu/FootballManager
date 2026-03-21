import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

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

    Rectangle {
        anchors.fill: parent
        color: "#f5f5f5"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 18

            Rectangle {
                Layout.fillWidth: true
                radius: 14
                color: "#ffffff"
                border.color: "#d9dee7"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Label {
                                text: gameFacade.getSelectedTeamName() || "Team"
                                font.pixelSize: 30
                                font.bold: true
                                color: "#1f2937"
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Label {
                                text: "Team overview, season stats, fixtures and squad information."
                                color: "#667085"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        Button {
                            text: "Back"
                            Layout.preferredHeight: 42
                            onClicked: root.backRequested()
                        }
                    }
                }
            }

            Label {
                visible: !root.hasActiveGame
                text: "No active game. Start a new game to view team details."
                color: "#667085"
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            ScrollView {
                id: teamScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.hasActiveGame
                clip: true

                Item {
                    width: teamScroll.availableWidth
                    implicitHeight: contentColumn.implicitHeight

                    ColumnLayout {
                        id: contentColumn
                        width: parent.width
                        spacing: 18

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 14
                            color: "#ffffff"
                            border.color: "#d9dee7"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 14

                                Label {
                                    text: "Team Season Stats"
                                    font.pixelSize: 21
                                    font.bold: true
                                    color: "#1f2937"
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 4
                                    rowSpacing: 12
                                    columnSpacing: 18

                                    Repeater {
                                        model: [
                                            ["Played", root.mapValue(root.seasonStats, "played", 0)],
                                            ["Wins", root.mapValue(root.seasonStats, "wins", 0)],
                                            ["Draws", root.mapValue(root.seasonStats, "draws", 0)],
                                            ["Losses", root.mapValue(root.seasonStats, "losses", 0)],
                                            ["Goals For", root.mapValue(root.seasonStats, "goalsFor", 0)],
                                            ["Goals Against", root.mapValue(root.seasonStats, "goalsAgainst", 0)],
                                            ["Clean Sheets", root.mapValue(root.seasonStats, "cleanSheets", 0)],
                                            ["Failed To Score", root.mapValue(root.seasonStats, "failedToScore", 0)]
                                        ]

                                        delegate: Rectangle {
                                            required property var modelData
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 82
                                            radius: 12
                                            color: "#f8fafc"
                                            border.color: "#e4e7ec"

                                            ColumnLayout {
                                                anchors.fill: parent
                                                anchors.margins: 12
                                                spacing: 4

                                                Label {
                                                    text: modelData[0]
                                                    font.bold: true
                                                    color: "#344054"
                                                    wrapMode: Text.WordWrap
                                                    Layout.fillWidth: true
                                                }

                                                Label {
                                                    text: modelData[1]
                                                    font.pixelSize: 22
                                                    font.bold: true
                                                    color: "#1f2937"
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 14
                            color: "#ffffff"
                            border.color: "#d9dee7"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 12

                                Label {
                                    text: "Recent / Played Matches"
                                    font.pixelSize: 21
                                    font.bold: true
                                    color: "#1f2937"
                                }

                                Repeater {
                                    model: root.recentMatches

                                    delegate: Rectangle {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        radius: 12
                                        color: "#f8fafc"
                                        border.color: "#e4e7ec"
                                        implicitHeight: 74

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 12
                                            spacing: 6

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    Layout.preferredWidth: 120
                                                    text: modelData.dateText || "-"
                                                    color: "#475467"
                                                }

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: modelData.opponentName || "Unknown opponent"
                                                    color: "#1f2937"
                                                    font.bold: true
                                                    elide: Text.ElideRight
                                                }

                                                Label {
                                                    text: (modelData.goalsFor || 0) + " - " + (modelData.goalsAgainst || 0)
                                                    font.bold: true
                                                    color: "#1f2937"
                                                }
                                            }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    text: modelData.isHome ? "Home" : "Away"
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: "Result: " + (modelData.resultLetter || "-")
                                                    color: (modelData.resultLetter || "") === "W" ? "#067647"
                                                         : (modelData.resultLetter || "") === "L" ? "#b42318"
                                                         : "#667085"
                                                    font.bold: true
                                                }

                                                Label {
                                                    text: "Matchweek " + (modelData.matchweek || 0)
                                                    color: "#667085"
                                                }

                                                Item { Layout.fillWidth: true }
                                            }
                                        }
                                    }
                                }

                                Label {
                                    visible: root.recentMatches.length === 0
                                    text: "No played matches available yet."
                                    color: "#667085"
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 14
                            color: "#ffffff"
                            border.color: "#d9dee7"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 12

                                Label {
                                    text: "Upcoming Matches"
                                    font.pixelSize: 21
                                    font.bold: true
                                    color: "#1f2937"
                                }

                                Repeater {
                                    model: root.upcomingMatches

                                    delegate: Rectangle {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        radius: 12
                                        color: "#f8fafc"
                                        border.color: "#e4e7ec"
                                        implicitHeight: 70

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 12
                                            spacing: 6

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    Layout.preferredWidth: 120
                                                    text: modelData.dateText || "-"
                                                    color: "#475467"
                                                }

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: (modelData.homeTeamName || "") + " vs " + (modelData.awayTeamName || "")
                                                    color: "#1f2937"
                                                    font.bold: true
                                                    wrapMode: Text.WordWrap
                                                }
                                            }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    text: modelData.isHome ? "Home" : "Away"
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: "Matchweek " + (modelData.matchweek || 0)
                                                    color: "#667085"
                                                }

                                                Item { Layout.fillWidth: true }
                                            }
                                        }
                                    }
                                }

                                Label {
                                    visible: root.upcomingMatches.length === 0
                                    text: "No upcoming match scheduled."
                                    color: "#667085"
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 14
                            color: "#ffffff"
                            border.color: "#d9dee7"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 12

                                Label {
                                    text: "Players List"
                                    font.pixelSize: 21
                                    font.bold: true
                                    color: "#1f2937"
                                }

                                Repeater {
                                    model: root.players

                                    delegate: Rectangle {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        radius: 12
                                        color: "#f8fafc"
                                        border.color: "#e4e7ec"
                                        implicitHeight: 78

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.openPlayerDetails(parent.modelData.playerId)
                                        }

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 12
                                            spacing: 6

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: modelData.name || "Unknown"
                                                    font.bold: true
                                                    color: "#1f2937"
                                                    elide: Text.ElideRight
                                                }

                                                Label {
                                                    text: modelData.position || "-"
                                                    color: "#475467"
                                                }
                                            }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    text: "Age: " + (modelData.age || 0)
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: modelData.overallSummary || "-"
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: "Apps " + (modelData.appearances || 0)
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: "G " + (modelData.goals || 0)
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: "A " + (modelData.assists || 0)
                                                    color: "#667085"
                                                }

                                                Item { Layout.fillWidth: true }

                                                Label {
                                                    text: "View details"
                                                    color: "#2f5fbf"
                                                    font.bold: true
                                                }
                                            }
                                        }
                                    }
                                }

                                Label {
                                    visible: root.players.length === 0
                                    text: "No player data available."
                                    color: "#667085"
                                }
                            }
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

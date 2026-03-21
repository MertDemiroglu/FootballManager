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
        seasonStats = hasActiveGame ? gameFacade.getCurrentTeamSeasonStats() : ({})
        recentMatches = hasActiveGame ? gameFacade.getCurrentTeamMatches() : []
        upcomingMatches = hasActiveGame ? gameFacade.getCurrentTeamUpcomingMatches(5) : []
        players = hasActiveGame ? gameFacade.getCurrentTeamPlayers() : []
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

    function resultColor(resultLetter) {
        if (resultLetter === "W") {
            return "#067647"
        }
        if (resultLetter === "D") {
            return "#b54708"
        }
        if (resultLetter === "L") {
            return "#b42318"
        }
        return "#667085"
    }

    function resultBackground(resultLetter) {
        if (resultLetter === "W") {
            return "#ecfdf3"
        }
        if (resultLetter === "D") {
            return "#fffaeb"
        }
        if (resultLetter === "L") {
            return "#fef3f2"
        }
        return "#f2f4f7"
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
        color: "#f4f6fb"

        ColumnLayout {
            anchors.fill: parent
            spacing: 18

            Rectangle {
                Layout.fillWidth: true
                radius: 16
                color: "#ffffff"
                border.color: "#d8dee8"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 14

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Label {
                                text: gameFacade.getSelectedTeamName() || "Team"
                                font.pixelSize: 32
                                font.bold: true
                                color: "#17212f"
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Label {
                                text: "Team overview, season stats, fixtures and squad information."
                                color: "#526071"
                                font.pixelSize: 15
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        Button {
                            text: "Back"
                            Layout.preferredHeight: 44
                            font.pixelSize: 15
                            onClicked: root.backRequested()
                        }
                    }
                }
            }

            Label {
                visible: !root.hasActiveGame
                text: "No active game. Start a new game to view team details."
                color: "#667085"
                font.pixelSize: 16
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
                    width: Math.max(teamScroll.availableWidth, 860)
                    implicitHeight: contentColumn.implicitHeight

                    ColumnLayout {
                        id: contentColumn
                        width: parent.width
                        spacing: 18

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 16
                            color: "#ffffff"
                            border.color: "#d8dee8"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 14

                                Label {
                                    text: "Team Season Stats"
                                    font.pixelSize: 22
                                    font.bold: true
                                    color: "#17212f"
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: width >= 760 ? 4 : 2
                                    rowSpacing: 12
                                    columnSpacing: 14

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
                                            Layout.preferredHeight: 90
                                            radius: 12
                                            color: "#f8fafc"
                                            border.color: "#e4e7ec"

                                            ColumnLayout {
                                                anchors.fill: parent
                                                anchors.margins: 12
                                                spacing: 6

                                                Label {
                                                    text: modelData[0]
                                                    font.bold: true
                                                    font.pixelSize: 15
                                                    color: "#344054"
                                                    wrapMode: Text.WordWrap
                                                    Layout.fillWidth: true
                                                }

                                                Label {
                                                    text: modelData[1]
                                                    font.pixelSize: 24
                                                    font.bold: true
                                                    color: "#17212f"
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 16
                            color: "#ffffff"
                            border.color: "#d8dee8"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 12

                                Label {
                                    text: "Recent Matches"
                                    font.pixelSize: 22
                                    font.bold: true
                                    color: "#17212f"
                                }

                                Repeater {
                                    model: root.recentMatches

                                    delegate: Rectangle {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        radius: 12
                                        color: "#f8fafc"
                                        border.color: "#e4e7ec"
                                        implicitHeight: 96

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 14
                                            spacing: 8

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    Layout.preferredWidth: 140
                                                    text: modelData.dateText || "-"
                                                    font.pixelSize: 14
                                                    color: "#475467"
                                                }

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: modelData.opponentName || "Unknown opponent"
                                                    font.pixelSize: 16
                                                    font.bold: true
                                                    color: "#17212f"
                                                    elide: Text.ElideRight
                                                }

                                                Rectangle {
                                                    radius: 999
                                                    color: root.resultBackground(modelData.resultLetter || "")
                                                    border.color: root.resultColor(modelData.resultLetter || "")
                                                    implicitWidth: 38
                                                    implicitHeight: 30

                                                    Label {
                                                        anchors.centerIn: parent
                                                        text: modelData.resultLetter || "-"
                                                        font.pixelSize: 14
                                                        font.bold: true
                                                        color: root.resultColor(modelData.resultLetter || "")
                                                    }
                                                }
                                            }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    text: "Score: " + (modelData.goalsFor || 0) + " - " + (modelData.goalsAgainst || 0)
                                                    font.pixelSize: 15
                                                    font.bold: true
                                                    color: "#17212f"
                                                }

                                                Label {
                                                    text: modelData.isHome ? "Home" : "Away"
                                                    font.pixelSize: 14
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: "Matchweek " + (modelData.matchweek || 0)
                                                    font.pixelSize: 14
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
                                    font.pixelSize: 15
                                    color: "#667085"
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 16
                            color: "#ffffff"
                            border.color: "#d8dee8"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 12

                                Label {
                                    text: "Upcoming Matches"
                                    font.pixelSize: 22
                                    font.bold: true
                                    color: "#17212f"
                                }

                                Repeater {
                                    model: root.upcomingMatches

                                    delegate: Rectangle {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        radius: 12
                                        color: "#f8fafc"
                                        border.color: "#e4e7ec"
                                        implicitHeight: 92

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 14
                                            spacing: 8

                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.dateText || "-"
                                                font.pixelSize: 14
                                                color: "#475467"
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: (modelData.homeTeamName || "") + " vs " + (modelData.awayTeamName || "")
                                                font.pixelSize: 16
                                                font.bold: true
                                                color: "#17212f"
                                                wrapMode: Text.WordWrap
                                            }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    text: modelData.isHome ? "Home" : "Away"
                                                    font.pixelSize: 14
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: "Matchweek " + (modelData.matchweek || 0)
                                                    font.pixelSize: 14
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
                                    font.pixelSize: 15
                                    color: "#667085"
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 16
                            color: "#ffffff"
                            border.color: "#d8dee8"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 12

                                Label {
                                    text: "Players List"
                                    font.pixelSize: 22
                                    font.bold: true
                                    color: "#17212f"
                                }

                                Repeater {
                                    model: root.players

                                    delegate: Rectangle {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        radius: 12
                                        color: "#f8fafc"
                                        border.color: "#e4e7ec"
                                        implicitHeight: 106

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.openPlayerDetails(parent.modelData.playerId)
                                        }

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 14
                                            spacing: 8

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: modelData.name || "Unknown"
                                                    font.pixelSize: 16
                                                    font.bold: true
                                                    color: "#17212f"
                                                    elide: Text.ElideRight
                                                }

                                                Label {
                                                    text: modelData.position || "-"
                                                    font.pixelSize: 14
                                                    color: "#475467"
                                                }
                                            }

                                            Flow {
                                                width: parent.width
                                                spacing: 12

                                                Label { text: "Age: " + (modelData.age || 0); font.pixelSize: 14; color: "#667085" }
                                                Label { text: modelData.overallSummary || "-"; font.pixelSize: 14; color: "#667085" }
                                                Label { text: "Apps " + (modelData.appearances || 0); font.pixelSize: 14; color: "#667085" }
                                                Label { text: "G " + (modelData.goals || 0); font.pixelSize: 14; color: "#667085" }
                                                Label { text: "A " + (modelData.assists || 0); font.pixelSize: 14; color: "#667085" }
                                            }

                                            Label {
                                                text: "View details"
                                                font.pixelSize: 14
                                                font.bold: true
                                                color: "#2f5fbf"
                                            }
                                        }
                                    }
                                }

                                Label {
                                    visible: root.players.length === 0
                                    text: "No player data available."
                                    font.pixelSize: 15
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

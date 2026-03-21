import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    signal backRequested()

    readonly property int pagePadding: 24
    readonly property int sectionSpacing: 20

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

    function hasMapValue(mapObject, key) {
        return !!mapObject && mapObject[key] !== undefined && mapObject[key] !== null
    }

    function displayValue(mapObject, key) {
        return hasMapValue(mapObject, key) ? String(mapObject[key]) : "—"
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
            anchors.margins: root.pagePadding
            spacing: root.sectionSpacing

            Rectangle {
                Layout.fillWidth: true
                radius: 18
                color: "#ffffff"
                border.color: "#d8dee8"
                implicitHeight: headerCardContent.implicitHeight + 40

                RowLayout {
                    id: headerCardContent
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 16

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: gameFacade.getSelectedTeamName() || "Team"
                            font.pixelSize: 34
                            font.bold: true
                            color: "#17212f"
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label {
                            text: root.hasActiveGame
                                  ? "Team overview, season stats, fixtures and squad information for the current club."
                                  : "Start a new game to load the team overview, fixtures and squad information."
                            color: "#526071"
                            font.pixelSize: 16
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    Button {
                        text: "Back"
                        Layout.preferredHeight: 46
                        Layout.preferredWidth: 108
                        font.pixelSize: 15
                        onClicked: root.backRequested()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 18
                color: "#ffffff"
                border.color: "#d8dee8"
                implicitHeight: emptyStateContent.implicitHeight + 40
                visible: !root.hasActiveGame

                Column {
                    id: emptyStateContent
                    x: 24
                    y: 20
                    width: Math.max(0, parent.width - 48)
                    spacing: 10

                    Label {
                        width: parent.width
                        text: "No active game"
                        color: "#17212f"
                        font.pixelSize: 24
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }

                    Label {
                        width: parent.width
                        text: "Go back to the home flow, start a save and this page will populate from gameFacade calls only."
                        color: "#667085"
                        font.pixelSize: 16
                        wrapMode: Text.WordWrap
                    }
                }
            }

            ScrollView {
                id: teamScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.hasActiveGame
                clip: true
                contentWidth: availableWidth

                Item {
                    width: teamScroll.availableWidth
                    implicitHeight: contentColumn.implicitHeight

                    Column {
                        id: contentColumn
                        width: parent.width
                        spacing: root.sectionSpacing

                        Rectangle {
                            width: parent.width
                            radius: 18
                            color: "#ffffff"
                            border.color: "#d8dee8"
                            implicitHeight: seasonStatsContent.implicitHeight + 40

                            ColumnLayout {
                                id: seasonStatsContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 16

                                Label {
                                    text: "Team Season Stats"
                                    font.pixelSize: 24
                                    font.bold: true
                                    color: "#17212f"
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: width >= 840 ? 4 : 2
                                    rowSpacing: 12
                                    columnSpacing: 14

                                    Repeater {
                                        model: [
                                            ["Played", root.displayValue(root.seasonStats, "played")],
                                            ["Wins", root.displayValue(root.seasonStats, "wins")],
                                            ["Draws", root.displayValue(root.seasonStats, "draws")],
                                            ["Losses", root.displayValue(root.seasonStats, "losses")],
                                            ["Goals For", root.displayValue(root.seasonStats, "goalsFor")],
                                            ["Goals Against", root.displayValue(root.seasonStats, "goalsAgainst")],
                                            ["Clean Sheets", root.displayValue(root.seasonStats, "cleanSheets")],
                                            ["Failed To Score", root.displayValue(root.seasonStats, "failedToScore")]
                                        ]

                                        delegate: Rectangle {
                                            required property var modelData
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 96
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
                            width: parent.width
                            radius: 18
                            color: "#ffffff"
                            border.color: "#d8dee8"
                            implicitHeight: recentMatchesContent.implicitHeight + 40

                            ColumnLayout {
                                id: recentMatchesContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 12

                                Label {
                                    text: "Recent Matches"
                                    font.pixelSize: 24
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
                                        implicitHeight: recentMatchDelegateContent.implicitHeight + 28

                                        ColumnLayout {
                                            id: recentMatchDelegateContent
                                            anchors.fill: parent
                                            anchors.margins: 14
                                            spacing: 10

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    text: modelData.dateText || "—"
                                                    font.pixelSize: 15
                                                    color: "#475467"
                                                    Layout.preferredWidth: 150
                                                }

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: modelData.opponentName || "Unknown opponent"
                                                    font.pixelSize: 17
                                                    font.bold: true
                                                    color: "#17212f"
                                                    elide: Text.ElideRight
                                                }

                                                Rectangle {
                                                    radius: 999
                                                    color: root.resultBackground(modelData.resultLetter || "")
                                                    border.color: root.resultColor(modelData.resultLetter || "")
                                                    implicitWidth: 42
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

                                            Flow {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    text: "Score: " + root.displayValue(modelData, "goalsFor") + " - " + root.displayValue(modelData, "goalsAgainst")
                                                    font.pixelSize: 15
                                                    font.bold: true
                                                    color: "#17212f"
                                                }

                                                Label {
                                                    text: modelData.isHome ? "Home" : "Away"
                                                    font.pixelSize: 15
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: "Matchweek " + (modelData.matchweek || "—")
                                                    font.pixelSize: 15
                                                    color: "#667085"
                                                }
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
                            width: parent.width
                            radius: 18
                            color: "#ffffff"
                            border.color: "#d8dee8"
                            implicitHeight: upcomingMatchesContent.implicitHeight + 40

                            ColumnLayout {
                                id: upcomingMatchesContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 12

                                Label {
                                    text: "Upcoming Matches"
                                    font.pixelSize: 24
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
                                        implicitHeight: upcomingMatchDelegateContent.implicitHeight + 28

                                        ColumnLayout {
                                            id: upcomingMatchDelegateContent
                                            anchors.fill: parent
                                            anchors.margins: 14
                                            spacing: 8

                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.dateText || "—"
                                                font.pixelSize: 15
                                                color: "#475467"
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: (modelData.homeTeamName || "") + " vs " + (modelData.awayTeamName || "")
                                                font.pixelSize: 17
                                                font.bold: true
                                                color: "#17212f"
                                                wrapMode: Text.WordWrap
                                            }

                                            Flow {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    text: modelData.isHome ? "Home" : "Away"
                                                    font.pixelSize: 15
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: "Matchweek " + (modelData.matchweek || "—")
                                                    font.pixelSize: 15
                                                    color: "#667085"
                                                }
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
                            width: parent.width
                            radius: 18
                            color: "#ffffff"
                            border.color: "#d8dee8"
                            implicitHeight: playersContent.implicitHeight + 40

                            ColumnLayout {
                                id: playersContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 12

                                Label {
                                    text: "Players List"
                                    font.pixelSize: 24
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
                                        implicitHeight: playerDelegateContent.implicitHeight + 28

                                        ColumnLayout {
                                            id: playerDelegateContent
                                            anchors.fill: parent
                                            anchors.margins: 14
                                            spacing: 10

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: modelData.name || "Unknown"
                                                    font.pixelSize: 17
                                                    font.bold: true
                                                    color: "#17212f"
                                                    elide: Text.ElideRight
                                                }

                                                Label {
                                                    text: modelData.position || "—"
                                                    font.pixelSize: 15
                                                    color: "#475467"
                                                }

                                                Button {
                                                    text: "Details"
                                                    Layout.preferredHeight: 40
                                                    font.pixelSize: 14
                                                    onClicked: root.openPlayerDetails(modelData.playerId)
                                                }
                                            }

                                            Flow {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label { text: "Age: " + root.displayValue(modelData, "age"); font.pixelSize: 15; color: "#667085" }
                                                Label { text: modelData.overallSummary || "—"; font.pixelSize: 15; color: "#667085" }
                                                Label { text: "Apps " + root.displayValue(modelData, "appearances"); font.pixelSize: 15; color: "#667085" }
                                                Label { text: "G " + root.displayValue(modelData, "goals"); font.pixelSize: 15; color: "#667085" }
                                                Label { text: "A " + root.displayValue(modelData, "assists"); font.pixelSize: 15; color: "#667085" }
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "UiHelpers.js" as UiHelpers

Item {
    id: root
    anchors.fill: parent

    signal backRequested()
    signal openMatchDetailRequested(var matchId)

    readonly property int pagePadding: 24
    readonly property int sectionSpacing: 20

    property var selectedPlayerDetails: ({})
    readonly property var shellState: gameFacade.shellState
    readonly property bool hasActiveGame: shellState.hasStartedGame

    function seasonStatText(value) {
        const statsObject = gameFacade.currentTeamSeasonStatsObject
        if (!statsObject || !statsObject.hasData) {
            return "—"
        }
        return String(value)
    }

    function openPlayerDetails(playerId) {
        if (!root.hasActiveGame) {
            return
        }
        selectedPlayerDetails = gameFacade.getPlayerDetails(playerId)
        playerDialog.open()
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
                            text: root.shellState.selectedTeamName || "Team"
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
                                            ["Played", root.seasonStatText(gameFacade.currentTeamSeasonStatsObject.played)],
                                            ["Wins", root.seasonStatText(gameFacade.currentTeamSeasonStatsObject.wins)],
                                            ["Draws", root.seasonStatText(gameFacade.currentTeamSeasonStatsObject.draws)],
                                            ["Losses", root.seasonStatText(gameFacade.currentTeamSeasonStatsObject.losses)],
                                            ["Goals For", root.seasonStatText(gameFacade.currentTeamSeasonStatsObject.goalsFor)],
                                            ["Goals Against", root.seasonStatText(gameFacade.currentTeamSeasonStatsObject.goalsAgainst)],
                                            ["Clean Sheets", root.seasonStatText(gameFacade.currentTeamSeasonStatsObject.cleanSheets)],
                                            ["Failed To Score", root.seasonStatText(gameFacade.currentTeamSeasonStatsObject.failedToScore)]
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
                                    id: recentMatchesRepeater
                                    model: gameFacade.currentTeamRecentMatchesModel

                                    delegate: Rectangle {
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
                                                    text: dateText || "—"
                                                    font.pixelSize: 15
                                                    color: "#475467"
                                                    Layout.preferredWidth: 150
                                                }

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: opponentName || "Unknown opponent"
                                                    font.pixelSize: 17
                                                    font.bold: true
                                                    color: "#17212f"
                                                    elide: Text.ElideRight
                                                }

                                                Rectangle {
                                                    radius: 999
                                                    color: UiHelpers.resultBackground(resultLetter || "")
                                                    border.color: UiHelpers.resultColor(resultLetter || "")
                                                    implicitWidth: 42
                                                    implicitHeight: 30

                                                    Label {
                                                        anchors.centerIn: parent
                                                        text: resultLetter || "-"
                                                        font.pixelSize: 14
                                                        font.bold: true
                                                        color: UiHelpers.resultColor(resultLetter || "")
                                                    }
                                                }
                                            }

                                            Flow {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    text: "Score: " + goalsFor + " - " + goalsAgainst
                                                    font.pixelSize: 15
                                                    font.bold: true
                                                    color: "#17212f"
                                                }

                                                Label {
                                                    text: isHome ? "Home" : "Away"
                                                    font.pixelSize: 15
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: "Matchweek " + (matchweek || "—")
                                                    font.pixelSize: 15
                                                    color: "#667085"
                                                }

                                                Button {
                                                    text: "Details"
                                                    enabled: (matchId || 0) > 0
                                                    onClicked: root.openMatchDetailRequested(matchId || 0)
                                                }
                                            }
                                        }
                                    }
                                }

                                Label {
                                    visible: recentMatchesRepeater.count === 0
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
                                    id: upcomingMatchesRepeater
                                    model: gameFacade.currentTeamUpcomingMatchesModel

                                    delegate: Rectangle {
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
                                                text: dateText || "—"
                                                font.pixelSize: 15
                                                color: "#475467"
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: (homeTeamName || "") + " vs " + (awayTeamName || "")
                                                font.pixelSize: 17
                                                font.bold: true
                                                color: "#17212f"
                                                wrapMode: Text.WordWrap
                                            }

                                            Flow {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label {
                                                    text: isHome ? "Home" : "Away"
                                                    font.pixelSize: 15
                                                    color: "#667085"
                                                }

                                                Label {
                                                    text: "Matchweek " + (matchweek || "—")
                                                    font.pixelSize: 15
                                                    color: "#667085"
                                                }
                                            }
                                        }
                                    }
                                }

                                Label {
                                    visible: upcomingMatchesRepeater.count === 0
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
                                    id: playersRepeater
                                    model: gameFacade.currentTeamPlayersModel

                                    delegate: Rectangle {
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
                                                    text: name || "Unknown"
                                                    font.pixelSize: 17
                                                    font.bold: true
                                                    color: "#17212f"
                                                    elide: Text.ElideRight
                                                }

                                                Label {
                                                    text: position || "—"
                                                    font.pixelSize: 15
                                                    color: "#475467"
                                                }

                                                Button {
                                                    text: "Details"
                                                    Layout.preferredHeight: 40
                                                    font.pixelSize: 14
                                                    onClicked: root.openPlayerDetails(playerId)
                                                }
                                            }

                                            Flow {
                                                Layout.fillWidth: true
                                                spacing: 12

                                                Label { text: "Age: " + age; font.pixelSize: 15; color: "#667085" }
                                                Label { text: overallSummary || "—"; font.pixelSize: 15; color: "#667085" }
                                                Label { text: "Apps " + appearances; font.pixelSize: 15; color: "#667085" }
                                                Label { text: "G " + goals; font.pixelSize: 15; color: "#667085" }
                                                Label { text: "A " + assists; font.pixelSize: 15; color: "#667085" }
                                            }
                                        }
                                    }
                                }

                                Label {
                                    visible: playersRepeater.count === 0
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

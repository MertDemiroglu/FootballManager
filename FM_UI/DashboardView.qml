import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    signal openStandingsRequested()
    signal openTeamRequested()

    property var dashboard: ({})
    property bool hasActiveGame: gameFacade.hasStartedGame()

    function refreshData() {
        hasActiveGame = gameFacade.hasStartedGame()
        dashboard = gameFacade.getDashboard()
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
                visible: root.hasActiveGame

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 14

                    Label {
                        text: root.mapValue(root.dashboard, "selectedTeamName", "No Team")
                        font.pixelSize: 30
                        font.bold: true
                        color: "#1f2937"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 4
                        rowSpacing: 8
                        columnSpacing: 16

                        Label {
                            text: "Date"
                            font.bold: true
                            color: "#344054"
                        }
                        Label {
                            text: root.mapValue(root.dashboard, "currentDateText", "-")
                            color: "#475467"
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label {
                            text: "State"
                            font.bold: true
                            color: "#344054"
                        }
                        Label {
                            text: root.mapValue(root.dashboard, "gameStateText", "-")
                            color: "#475467"
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label {
                            text: "Manager"
                            font.bold: true
                            color: "#344054"
                        }
                        Label {
                            text: gameFacade.getManagerName() || "-"
                            color: "#475467"
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label {
                            text: "Club"
                            font.bold: true
                            color: "#344054"
                        }
                        Label {
                            text: root.mapValue(root.dashboard, "selectedTeamName", "-")
                            color: "#475467"
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            Label {
                visible: !root.hasActiveGame
                text: "Start a new game from the home screen to view the dashboard."
                color: "#667085"
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 18
                visible: root.hasActiveGame

                Rectangle {
                    id: scheduleCard
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    radius: 14
                    color: "#ffffff"
                    border.color: "#d9dee7"

                    property var nextMatch: root.mapValue(root.dashboard, "nextMatch", {})
                    property var upcomingMatches: root.mapValue(root.dashboard, "upcomingMatches", [])

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 16

                        Label {
                            text: "Schedule Overview"
                            font.pixelSize: 21
                            font.bold: true
                            color: "#1f2937"
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Label {
                                text: "Next Match"
                                font.pixelSize: 15
                                font.bold: true
                                color: "#344054"
                            }

                            Label {
                                Layout.fillWidth: true
                                text: scheduleCard.nextMatch && scheduleCard.nextMatch.hasNextMatch
                                      ? (scheduleCard.nextMatch.dateText || "-") + " • "
                                        + (scheduleCard.nextMatch.homeTeamName || "") + " vs "
                                        + (scheduleCard.nextMatch.awayTeamName || "")
                                      : "No upcoming match"
                                wrapMode: Text.WordWrap
                                color: "#475467"
                            }

                            Label {
                                Layout.fillWidth: true
                                text: scheduleCard.nextMatch && scheduleCard.nextMatch.hasNextMatch
                                      ? ((scheduleCard.nextMatch.isHome ? "Home" : "Away")
                                         + " • Matchweek " + (scheduleCard.nextMatch.matchweek || 0))
                                      : ""
                                color: "#667085"
                                visible: text.length > 0
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#eaecf0"
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Label {
                                text: "Recent Form"
                                font.pixelSize: 15
                                font.bold: true
                                color: "#344054"
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.mapValue(root.dashboard, "recentForm", "") || "No data"
                                font.pixelSize: 22
                                font.bold: true
                                color: "#1d4ed8"
                                wrapMode: Text.WordWrap
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#eaecf0"
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 8

                            Label {
                                text: "Upcoming Matches"
                                font.pixelSize: 15
                                font.bold: true
                                color: "#344054"
                            }

                            Repeater {
                                model: scheduleCard.upcomingMatches

                                delegate: Rectangle {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    radius: 10
                                    color: "#f8fafc"
                                    border.color: "#e4e7ec"
                                    implicitHeight: 58

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 12
                                        spacing: 10

                                        Label {
                                            Layout.preferredWidth: 110
                                            text: modelData.dateText || "-"
                                            color: "#344054"
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: (modelData.homeTeamName || "") + " vs " + (modelData.awayTeamName || "")
                                            wrapMode: Text.WordWrap
                                            color: "#1f2937"
                                        }

                                        Label {
                                            text: (modelData.isHome ? "Home" : "Away") + " • MW " + (modelData.matchweek || 0)
                                            color: "#667085"
                                        }
                                    }
                                }
                            }

                            Label {
                                visible: scheduleCard.upcomingMatches.length === 0
                                text: "No upcoming match"
                                color: "#667085"
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    radius: 14
                    color: "#ffffff"
                    border.color: "#d9dee7"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 14
                        property var stats: root.mapValue(root.dashboard, "shortTeamStats", {})

                        Label {
                            text: "Team Stats"
                            font.pixelSize: 21
                            font.bold: true
                            color: "#1f2937"
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            rowSpacing: 10
                            columnSpacing: 18

                            Label { text: "Played"; font.bold: true; color: "#344054" }
                            Label { text: root.mapValue(parent.stats, "played", 0); color: "#475467" }

                            Label { text: "Wins"; font.bold: true; color: "#344054" }
                            Label { text: root.mapValue(parent.stats, "wins", 0); color: "#475467" }

                            Label { text: "Draws"; font.bold: true; color: "#344054" }
                            Label { text: root.mapValue(parent.stats, "draws", 0); color: "#475467" }

                            Label { text: "Losses"; font.bold: true; color: "#344054" }
                            Label { text: root.mapValue(parent.stats, "losses", 0); color: "#475467" }

                            Label { text: "Goals For"; font.bold: true; color: "#344054" }
                            Label { text: root.mapValue(parent.stats, "goalsFor", 0); color: "#475467" }

                            Label { text: "Goals Against"; font.bold: true; color: "#344054" }
                            Label { text: root.mapValue(parent.stats, "goalsAgainst", 0); color: "#475467" }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    radius: 14
                    color: "#ffffff"
                    border.color: "#d9dee7"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 14
                        property var row: root.mapValue(root.dashboard, "standingsRow", {})

                        Label {
                            text: "League Position"
                            font.pixelSize: 21
                            font.bold: true
                            color: "#1f2937"
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            rowSpacing: 10
                            columnSpacing: 18

                            Label { text: "Position"; font.bold: true; color: "#344054" }
                            Label { text: root.mapValue(parent.row, "position", "-"); color: "#475467" }

                            Label { text: "Points"; font.bold: true; color: "#344054" }
                            Label { text: root.mapValue(parent.row, "points", 0); color: "#475467" }

                            Label { text: "Goal Diff"; font.bold: true; color: "#344054" }
                            Label { text: root.mapValue(parent.row, "goalDifference", 0); color: "#475467" }

                            Label { text: "Played"; font.bold: true; color: "#344054" }
                            Label { text: root.mapValue(parent.row, "played", 0); color: "#475467" }

                            Label { text: "Record"; font.bold: true; color: "#344054" }
                            Label {
                                text: root.mapValue(parent.row, "wins", 0) + "W "
                                      + root.mapValue(parent.row, "draws", 0) + "D "
                                      + root.mapValue(parent.row, "losses", 0) + "L"
                                color: "#475467"
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                visible: root.hasActiveGame

                Button {
                    text: "Next Day"
                    Layout.preferredHeight: 42
                    onClicked: gameFacade.advanceOneDay()
                }

                Button {
                    text: "Advance 7 Days"
                    Layout.preferredHeight: 42
                    onClicked: gameFacade.advanceDays(7)
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Standings"
                    Layout.preferredHeight: 42
                    onClicked: root.openStandingsRequested()
                }

                Button {
                    text: "Team"
                    Layout.preferredHeight: 42
                    onClicked: root.openTeamRequested()
                }
            }
        }
    }
}

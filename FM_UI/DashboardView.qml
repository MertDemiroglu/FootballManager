import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: root
    signal openStandingsRequested()
    signal openTeamRequested()

    property var dashboard: ({})

    function refreshData() {
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

    contentWidth: availableWidth
    clip: true

    ColumnLayout {
        width: root.availableWidth
        spacing: 16

        Rectangle {
            Layout.fillWidth: true
            radius: 10
            color: "white"
            border.color: "#d8d8d8"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 8

                Label {
                    text: root.mapValue(root.dashboard, "selectedTeamName", "No Team")
                    font.pixelSize: 28
                    font.bold: true
                    color: "#202020"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 20

                    Label { text: "Date: " + root.mapValue(root.dashboard, "currentDateText", "-") }
                    Label { text: "State: " + root.mapValue(root.dashboard, "gameStateText", "-") }
                    Label { text: "Manager: " + (gameFacade.getManagerName() || "-") }
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
                    text: "Next Match"
                    font.pixelSize: 20
                    font.bold: true
                }

                property var nextMatch: root.mapValue(root.dashboard, "nextMatch", {})

                Label {
                    text: parent.nextMatch && parent.nextMatch.hasNextMatch
                          ? (parent.nextMatch.dateText || "") + " - "
                            + (parent.nextMatch.homeTeamName || "") + " vs "
                            + (parent.nextMatch.awayTeamName || "")
                          : "No upcoming match"
                    wrapMode: Text.WordWrap
                    color: "#303030"
                }

                Label {
                    text: parent.nextMatch && parent.nextMatch.hasNextMatch
                          ? ((parent.nextMatch.isHome ? "Home" : "Away")
                            + " · Matchweek " + (parent.nextMatch.matchweek || 0))
                          : ""
                    color: "#666666"
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                radius: 10
                color: "white"
                border.color: "#d8d8d8"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 8

                    Label {
                        text: "Recent Form"
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Label {
                        text: root.mapValue(root.dashboard, "recentForm", "") || "No data"
                        color: "#303030"
                        font.pixelSize: 20
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                radius: 10
                color: "white"
                border.color: "#d8d8d8"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 6
                    property var stats: root.mapValue(root.dashboard, "shortTeamStats", {})

                    Label {
                        text: "Team Stats"
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Label { text: "Played: " + root.mapValue(parent.stats, "played", 0) }
                    Label { text: "Record: " + root.mapValue(parent.stats, "wins", 0) + "W " + root.mapValue(parent.stats, "draws", 0) + "D " + root.mapValue(parent.stats, "losses", 0) + "L" }
                    Label { text: "Goals: " + root.mapValue(parent.stats, "goalsFor", 0) + " / " + root.mapValue(parent.stats, "goalsAgainst", 0) }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                radius: 10
                color: "white"
                border.color: "#d8d8d8"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 6
                    property var row: root.mapValue(root.dashboard, "standingsRow", {})

                    Label {
                        text: "League Position"
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Label { text: "Position: " + root.mapValue(parent.row, "position", "-") }
                    Label { text: "Points: " + root.mapValue(parent.row, "points", 0) }
                    Label { text: "Goal Diff: " + root.mapValue(parent.row, "goalDifference", 0) }
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
                    model: root.mapValue(root.dashboard, "upcomingMatches", [])

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
                            spacing: 14

                            Label {
                                Layout.preferredWidth: 160
                                text: modelData.dateText || "-"
                            }

                            Label {
                                Layout.fillWidth: true
                                text: (modelData.homeTeamName || "") + " vs " + (modelData.awayTeamName || "")
                                wrapMode: Text.WordWrap
                            }

                            Label {
                                text: (modelData.isHome ? "Home" : "Away") + " · MW " + (modelData.matchweek || 0)
                                color: "#666666"
                            }
                        }
                    }
                }

                Label {
                    visible: root.mapValue(root.dashboard, "upcomingMatches", []).length === 0
                    text: "No upcoming match"
                    color: "#666666"
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                text: "Next Day"
                onClicked: gameFacade.advanceOneDay()
            }

            Button {
                text: "Advance 7 Days"
                onClicked: gameFacade.advanceDays(7)
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Standings"
                onClicked: root.openStandingsRequested()
            }

            Button {
                text: "Team"
                onClicked: root.openTeamRequested()
            }
        }
    }
}
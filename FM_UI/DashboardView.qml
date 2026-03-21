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
    property var teamStats: mapValue(dashboard, "shortTeamStats", {})
    property var standingsRow: mapValue(dashboard, "standingsRow", {})
    property var nextMatch: mapValue(dashboard, "nextMatch", {})
    property var upcomingMatches: mapValue(dashboard, "upcomingMatches", [])

    function refreshData() {
        hasActiveGame = gameFacade.hasStartedGame()
        dashboard = hasActiveGame ? gameFacade.getDashboard() : ({})
    }

    function mapValue(mapObject, key, fallbackValue) {
        if (!mapObject || mapObject[key] === undefined || mapObject[key] === null) {
            return fallbackValue
        }
        return mapObject[key]
    }

    function formCharacters() {
        var rawForm = String(mapValue(dashboard, "recentForm", "") || "")
        if (!rawForm.length) {
            return []
        }

        var cleaned = rawForm.replace(/[^WDL]/g, "")
        return cleaned.length ? cleaned.split("") : []
    }

    function formColor(resultLetter) {
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

    function formBackground(resultLetter) {
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

        ScrollView {
            anchors.fill: parent
            clip: true

            Item {
                width: Math.max(root.width, dashboardColumn.implicitWidth)
                implicitHeight: dashboardColumn.implicitHeight

                ColumnLayout {
                    id: dashboardColumn
                    width: Math.max(parent.width, 860)
                    spacing: 18

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 16
                        color: "#ffffff"
                        border.color: "#d8dee8"
                        visible: root.hasActiveGame

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 24
                            spacing: 16

                            Label {
                                text: root.mapValue(root.dashboard, "selectedTeamName", "No Team")
                                font.pixelSize: 32
                                font.bold: true
                                color: "#17212f"
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Label {
                                text: "Season overview, fixtures, form and standings snapshot for the current club."
                                color: "#526071"
                                font.pixelSize: 15
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: width >= 720 ? 4 : 2
                                rowSpacing: 10
                                columnSpacing: 18

                                Label { text: "Date"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                Label { text: root.mapValue(root.dashboard, "currentDateText", "-"); font.pixelSize: 15; color: "#475467"; Layout.fillWidth: true; elide: Text.ElideRight }
                                Label { text: "State"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                Label { text: root.mapValue(root.dashboard, "gameStateText", "-"); font.pixelSize: 15; color: "#475467"; Layout.fillWidth: true; elide: Text.ElideRight }
                                Label { text: "Manager"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                Label { text: gameFacade.getManagerName() || "-"; font.pixelSize: 15; color: "#475467"; Layout.fillWidth: true; elide: Text.ElideRight }
                                Label { text: "Club"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                Label { text: root.mapValue(root.dashboard, "selectedTeamName", "-"); font.pixelSize: 15; color: "#475467"; Layout.fillWidth: true; elide: Text.ElideRight }
                            }
                        }
                    }

                    Label {
                        visible: !root.hasActiveGame
                        text: "Start a new game from the home screen to view the dashboard."
                        color: "#667085"
                        font.pixelSize: 16
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        visible: root.hasActiveGame
                        columns: width >= 1040 ? 3 : 1
                        rowSpacing: 18
                        columnSpacing: 18

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            radius: 16
                            color: "#ffffff"
                            border.color: "#d8dee8"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 16

                                Label {
                                    text: "Schedule Overview"
                                    font.pixelSize: 22
                                    font.bold: true
                                    color: "#17212f"
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Label {
                                        text: "Next Match"
                                        font.pixelSize: 16
                                        font.bold: true
                                        color: "#344054"
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: root.nextMatch && root.nextMatch.hasNextMatch
                                              ? (root.nextMatch.dateText || "-") + " • "
                                                + (root.nextMatch.homeTeamName || "") + " vs "
                                                + (root.nextMatch.awayTeamName || "")
                                              : "No upcoming match"
                                        wrapMode: Text.WordWrap
                                        font.pixelSize: 15
                                        color: "#475467"
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: root.nextMatch && root.nextMatch.hasNextMatch
                                              ? ((root.nextMatch.isHome ? "Home" : "Away")
                                                 + " • Matchweek " + (root.nextMatch.matchweek || 0))
                                              : ""
                                        font.pixelSize: 14
                                        color: "#667085"
                                        visible: text.length > 0
                                    }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: "#eaecf0" }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Label {
                                        text: "Recent Form"
                                        font.pixelSize: 16
                                        font.bold: true
                                        color: "#344054"
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8
                                        visible: root.formCharacters().length > 0

                                        Repeater {
                                            model: root.formCharacters()

                                            delegate: Rectangle {
                                                required property string modelData
                                                radius: 999
                                                color: root.formBackground(modelData)
                                                border.color: root.formColor(modelData)
                                                implicitWidth: 36
                                                implicitHeight: 36

                                                Label {
                                                    anchors.centerIn: parent
                                                    text: modelData
                                                    font.pixelSize: 15
                                                    font.bold: true
                                                    color: root.formColor(modelData)
                                                }
                                            }
                                        }

                                        Item { Layout.fillWidth: true }
                                    }

                                    Label {
                                        visible: root.formCharacters().length === 0
                                        text: "No data"
                                        font.pixelSize: 15
                                        color: "#667085"
                                    }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: "#eaecf0" }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Label {
                                        text: "Upcoming Matches"
                                        font.pixelSize: 16
                                        font.bold: true
                                        color: "#344054"
                                    }

                                    Repeater {
                                        model: root.upcomingMatches

                                        delegate: Rectangle {
                                            required property var modelData
                                            Layout.fillWidth: true
                                            radius: 12
                                            color: "#f8fafc"
                                            border.color: "#e4e7ec"
                                            implicitHeight: 72

                                            ColumnLayout {
                                                anchors.fill: parent
                                                anchors.margins: 12
                                                spacing: 6

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: modelData.dateText || "-"
                                                    font.pixelSize: 14
                                                    color: "#344054"
                                                }

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: (modelData.homeTeamName || "") + " vs " + (modelData.awayTeamName || "")
                                                    wrapMode: Text.WordWrap
                                                    font.pixelSize: 15
                                                    color: "#17212f"
                                                }

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: (modelData.isHome ? "Home" : "Away") + " • Matchweek " + (modelData.matchweek || 0)
                                                    font.pixelSize: 14
                                                    color: "#667085"
                                                }
                                            }
                                        }
                                    }

                                    Label {
                                        visible: root.upcomingMatches.length === 0
                                        text: "No upcoming match"
                                        font.pixelSize: 15
                                        color: "#667085"
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            radius: 16
                            color: "#ffffff"
                            border.color: "#d8dee8"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 14

                                Label {
                                    text: "Team Stats"
                                    font.pixelSize: 22
                                    font.bold: true
                                    color: "#17212f"
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 12
                                    columnSpacing: 18

                                    Label { text: "Played"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.teamStats, "played", 0); font.pixelSize: 16; color: "#475467" }
                                    Label { text: "Wins"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.teamStats, "wins", 0); font.pixelSize: 16; color: "#475467" }
                                    Label { text: "Draws"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.teamStats, "draws", 0); font.pixelSize: 16; color: "#475467" }
                                    Label { text: "Losses"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.teamStats, "losses", 0); font.pixelSize: 16; color: "#475467" }
                                    Label { text: "Goals For"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.teamStats, "goalsFor", 0); font.pixelSize: 16; color: "#475467" }
                                    Label { text: "Goals Against"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.teamStats, "goalsAgainst", 0); font.pixelSize: 16; color: "#475467" }
                                    Label { text: "Clean Sheets"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.teamStats, "cleanSheets", 0); font.pixelSize: 16; color: "#475467" }
                                    Label { text: "Failed To Score"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.teamStats, "failedToScore", 0); font.pixelSize: 16; color: "#475467" }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            radius: 16
                            color: "#ffffff"
                            border.color: "#d8dee8"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 14

                                Label {
                                    text: "League Position"
                                    font.pixelSize: 22
                                    font.bold: true
                                    color: "#17212f"
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 12
                                    columnSpacing: 18

                                    Label { text: "Position"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.standingsRow, "position", "-"); font.pixelSize: 16; color: "#475467" }
                                    Label { text: "Points"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.standingsRow, "points", 0); font.pixelSize: 16; color: "#475467" }
                                    Label { text: "Goal Diff"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.standingsRow, "goalDifference", 0); font.pixelSize: 16; color: "#475467" }
                                    Label { text: "Played"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label { text: root.mapValue(root.standingsRow, "played", 0); font.pixelSize: 16; color: "#475467" }
                                    Label { text: "Record"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label {
                                        text: root.mapValue(root.standingsRow, "wins", 0) + "W "
                                              + root.mapValue(root.standingsRow, "draws", 0) + "D "
                                              + root.mapValue(root.standingsRow, "losses", 0) + "L"
                                        font.pixelSize: 16
                                        color: "#475467"
                                    }
                                    Label { text: "Team"; font.bold: true; font.pixelSize: 15; color: "#344054" }
                                    Label {
                                        text: root.mapValue(root.standingsRow, "teamName", root.mapValue(root.dashboard, "selectedTeamName", "-"))
                                        font.pixelSize: 16
                                        color: "#475467"
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        visible: root.hasActiveGame

                        Button {
                            text: "Next Day"
                            Layout.preferredHeight: 44
                            font.pixelSize: 15
                            onClicked: gameFacade.advanceOneDay()
                        }

                        Button {
                            text: "Advance 7 Days"
                            Layout.preferredHeight: 44
                            font.pixelSize: 15
                            onClicked: gameFacade.advanceDays(7)
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: "Standings"
                            Layout.preferredHeight: 44
                            font.pixelSize: 15
                            onClicked: root.openStandingsRequested()
                        }

                        Button {
                            text: "Team"
                            Layout.preferredHeight: 44
                            font.pixelSize: 15
                            onClicked: root.openTeamRequested()
                        }
                    }
                }
            }
        }
    }
}

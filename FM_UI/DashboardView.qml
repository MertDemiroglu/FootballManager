import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    signal openStandingsRequested()
    signal openTeamRequested()
    signal pauseRequested()
    signal resumeRequested()

    readonly property int pagePadding: 24
    readonly property int sectionSpacing: 20

    property var dashboard: ({})
    property bool hasActiveGame: gameFacade.hasStartedGame()
    property var teamStats: mapValue(dashboard, "shortTeamStats", ({}))
    property var standingsRow: mapValue(dashboard, "standingsRow", ({}))
    property var nextMatch: mapValue(dashboard, "nextMatch", ({}))
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

    function hasMapValue(mapObject, key) {
        return !!mapObject && mapObject[key] !== undefined && mapObject[key] !== null
    }

    function displayValue(mapObject, key) {
        return hasMapValue(mapObject, key) ? String(mapObject[key]) : "—"
    }

    function formCharacters() {
        const rawForm = String(mapValue(dashboard, "recentForm", "") || "")
        if (!rawForm.length) {
            return []
        }

        const cleaned = rawForm.toUpperCase().replace(/[^WDL]/g, "")
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
            id: dashboardScroll
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth

            Item {
                width: dashboardScroll.availableWidth
                implicitHeight: pageColumn.implicitHeight + (root.pagePadding * 2)

                Column {
                    id: pageColumn
                    x: root.pagePadding
                    y: root.pagePadding
                    width: Math.max(0, parent.width - (root.pagePadding * 2))
                    spacing: root.sectionSpacing

                    Rectangle {
                        width: parent.width
                        radius: 18
                        color: "#ffffff"
                        border.color: "#d8dee8"
                        implicitHeight: headerContent.implicitHeight + 48
                        visible: root.hasActiveGame

                        ColumnLayout {
                            id: headerContent
                            anchors.fill: parent
                            anchors.margins: 24
                            spacing: 18

                            Label {
                                text: root.mapValue(root.dashboard, "selectedTeamName", "No Team")
                                font.pixelSize: 34
                                font.bold: true
                                color: "#17212f"
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Label {
                                text: "Season overview, fixtures, team form and league snapshot for the active club."
                                color: "#526071"
                                font.pixelSize: 16
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: width >= 760 ? 4 : 2
                                rowSpacing: 12
                                columnSpacing: 20

                                Label { text: "Date"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                Label {
                                    text: root.displayValue(root.dashboard, "currentDateText")
                                    font.pixelSize: 16
                                    color: "#475467"
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Label { text: "State"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                Label {
                                    text: root.displayValue(root.dashboard, "gameStateText")
                                    font.pixelSize: 16
                                    color: "#475467"
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Label { text: "Manager"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                Label {
                                    text: gameFacade.getManagerName() || "—"
                                    font.pixelSize: 16
                                    color: "#475467"
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Label { text: "Club"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                Label {
                                    text: root.displayValue(root.dashboard, "selectedTeamName")
                                    font.pixelSize: 16
                                    color: "#475467"
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
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
                                text: "Start a new game from the home screen to view the dashboard."
                                color: "#17212f"
                                font.pixelSize: 24
                                font.bold: true
                                wrapMode: Text.WordWrap
                            }

                            Label {
                                width: parent.width
                                text: "Once a club is selected, this page shows the next match, team season numbers and your current league position."
                                color: "#667085"
                                font.pixelSize: 16
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    GridLayout {
                        width: parent.width
                        visible: root.hasActiveGame
                        columns: width >= 1100 ? 3 : (width >= 720 ? 2 : 1)
                        rowSpacing: root.sectionSpacing
                        columnSpacing: root.sectionSpacing

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            radius: 18
                            color: "#ffffff"
                            border.color: "#d8dee8"
                            implicitHeight: scheduleCardContent.implicitHeight + 40

                            ColumnLayout {
                                id: scheduleCardContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 16

                                Label {
                                    text: "Schedule Overview"
                                    font.pixelSize: 24
                                    font.bold: true
                                    color: "#17212f"
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Label {
                                        text: "Next Match"
                                        font.pixelSize: 17
                                        font.bold: true
                                        color: "#344054"
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: root.nextMatch && root.nextMatch.hasNextMatch
                                              ? (root.nextMatch.dateText || "—") + " • "
                                                + (root.nextMatch.homeTeamName || "") + " vs "
                                                + (root.nextMatch.awayTeamName || "")
                                              : "No upcoming match"
                                        wrapMode: Text.WordWrap
                                        font.pixelSize: 16
                                        color: "#475467"
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: root.nextMatch && root.nextMatch.hasNextMatch
                                              ? ((root.nextMatch.isHome ? "Home" : "Away")
                                                 + " • Matchweek " + (root.nextMatch.matchweek || "—"))
                                              : ""
                                        font.pixelSize: 15
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
                                    spacing: 10

                                    Label {
                                        text: "Recent Form"
                                        font.pixelSize: 17
                                        font.bold: true
                                        color: "#344054"
                                    }

                                    Flow {
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
                                                width: 38
                                                height: 38

                                                Label {
                                                    anchors.centerIn: parent
                                                    text: modelData
                                                    font.pixelSize: 15
                                                    font.bold: true
                                                    color: root.formColor(modelData)
                                                }
                                            }
                                        }
                                    }

                                    Label {
                                        visible: root.formCharacters().length === 0
                                        text: "No data"
                                        font.pixelSize: 15
                                        color: "#667085"
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 1
                                    color: "#eaecf0"
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 10

                                    Label {
                                        text: "Upcoming Matches"
                                        font.pixelSize: 17
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
                                            implicitHeight: upcomingMatchContent.implicitHeight + 24

                                            ColumnLayout {
                                                id: upcomingMatchContent
                                                anchors.fill: parent
                                                anchors.margins: 12
                                                spacing: 6

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: modelData.dateText || "—"
                                                    font.pixelSize: 15
                                                    color: "#344054"
                                                }

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: (modelData.homeTeamName || "") + " vs " + (modelData.awayTeamName || "")
                                                    wrapMode: Text.WordWrap
                                                    font.pixelSize: 16
                                                    color: "#17212f"
                                                }

                                                Label {
                                                    Layout.fillWidth: true
                                                    text: (modelData.isHome ? "Home" : "Away") + " • Matchweek " + (modelData.matchweek || "—")
                                                    font.pixelSize: 15
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
                            radius: 18
                            color: "#ffffff"
                            border.color: "#d8dee8"
                            implicitHeight: teamStatsCardContent.implicitHeight + 40

                            ColumnLayout {
                                id: teamStatsCardContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 16

                                Label {
                                    text: "Team Stats"
                                    font.pixelSize: 24
                                    font.bold: true
                                    color: "#17212f"
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 12
                                    columnSpacing: 18

                                    Label { text: "Played"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.teamStats, "played"); font.pixelSize: 17; color: "#475467" }
                                    Label { text: "Wins"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.teamStats, "wins"); font.pixelSize: 17; color: "#475467" }
                                    Label { text: "Draws"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.teamStats, "draws"); font.pixelSize: 17; color: "#475467" }
                                    Label { text: "Losses"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.teamStats, "losses"); font.pixelSize: 17; color: "#475467" }
                                    Label { text: "Goals For"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.teamStats, "goalsFor"); font.pixelSize: 17; color: "#475467" }
                                    Label { text: "Goals Against"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.teamStats, "goalsAgainst"); font.pixelSize: 17; color: "#475467" }
                                    Label { text: "Goal Difference"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.teamStats, "goalDifference"); font.pixelSize: 17; color: "#475467" }
                                    Label { text: "Points"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.teamStats, "points"); font.pixelSize: 17; color: "#475467" }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            radius: 18
                            color: "#ffffff"
                            border.color: "#d8dee8"
                            implicitHeight: leagueCardContent.implicitHeight + 40

                            ColumnLayout {
                                id: leagueCardContent
                                anchors.fill: parent
                                anchors.margins: 20
                                spacing: 16

                                Label {
                                    text: "League Position"
                                    font.pixelSize: 24
                                    font.bold: true
                                    color: "#17212f"
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: 12
                                    columnSpacing: 18

                                    Label { text: "Position"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.standingsRow, "position"); font.pixelSize: 17; color: "#475467" }
                                    Label { text: "Points"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.standingsRow, "points"); font.pixelSize: 17; color: "#475467" }
                                    Label { text: "Played"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.standingsRow, "played"); font.pixelSize: 17; color: "#475467" }
                                    Label { text: "Goal Difference"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label { text: root.displayValue(root.standingsRow, "goalDifference"); font.pixelSize: 17; color: "#475467" }
                                    Label { text: "Record"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label {
                                        text: root.hasMapValue(root.standingsRow, "wins")
                                              ? root.displayValue(root.standingsRow, "wins") + "W "
                                                + root.displayValue(root.standingsRow, "draws") + "D "
                                                + root.displayValue(root.standingsRow, "losses") + "L"
                                              : "—"
                                        font.pixelSize: 17
                                        color: "#475467"
                                    }
                                    Label { text: "Team"; font.bold: true; font.pixelSize: 16; color: "#344054" }
                                    Label {
                                        text: root.hasMapValue(root.standingsRow, "teamName")
                                              ? root.displayValue(root.standingsRow, "teamName")
                                              : root.displayValue(root.dashboard, "selectedTeamName")
                                        font.pixelSize: 17
                                        color: "#475467"
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
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
                        implicitHeight: actionContent.implicitHeight + 32
                        visible: root.hasActiveGame

                        RowLayout {
                            id: actionContent
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 12

                            Button {
                                text: "Resume"
                                Layout.preferredHeight: 46
                                font.pixelSize: 15
                                enabled: root.hasActiveGame
                                onClicked: root.resumeRequested()
                            }

                            Button {
                                text: "Pause"
                                Layout.preferredHeight: 46
                                font.pixelSize: 15
                                enabled: root.hasActiveGame
                                onClicked: root.pauseRequested()
                            }

                            Button {
                                text: "Next Day (Debug)"
                                Layout.preferredHeight: 46
                                font.pixelSize: 15
                                onClicked: gameFacade.advanceOneDay()
                            }

                            Item { Layout.fillWidth: true }

                                                        Label {
                                text: gameFacade.hasActiveInteraction()
                                      ? "Interaction Active"
                                      : (gameFacade.isTimePaused() ? "Paused" : "Running")
                                color: gameFacade.hasActiveInteraction() ? "#b42318" : "#475467"
                                font.pixelSize: 14
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Button {
                                text: "Standings"
                                Layout.preferredHeight: 46
                                font.pixelSize: 15
                                onClicked: root.openStandingsRequested()
                            }

                            Button {
                                text: "Team"
                                Layout.preferredHeight: 46
                                font.pixelSize: 15
                                onClicked: root.openTeamRequested()
                            }
                        }
                    }
                }
            }
        }
    }
}

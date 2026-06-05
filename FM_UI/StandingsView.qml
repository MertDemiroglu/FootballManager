import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    signal backRequested()
    property var metrics: null

    readonly property var shellState: gameFacade.shellState
    readonly property bool hasActiveGame: shellState.hasStartedGame
    readonly property bool compactTable: metrics ? metrics.compact : width < 1400

    function columns() {
        return compactTable
            ? [ ["Pos", 50, "position"], ["Team", -1, "teamName"], ["GD", 60, "goalDifference"], ["Pts", 60, "points"] ]
            : [ ["Pos", 50, "position"], ["Team", -1, "teamName"], ["P", 50, "played"], ["W", 50, "wins"], ["D", 50, "draws"],
                ["L", 50, "losses"], ["GF", 60, "goalsFor"], ["GA", 60, "goalsAgainst"], ["GD", 60, "goalDifference"], ["Pts", 60, "points"] ]
    }

    function cellText(key, positionValue, teamValue, playedValue, winsValue, drawsValue, lossesValue, goalsForValue, goalsAgainstValue, goalDifferenceValue, pointsValue) {
        switch (key) {
        case "position": return positionValue || "-"
        case "teamName": return teamValue || "-"
        case "played": return playedValue || 0
        case "wins": return winsValue || 0
        case "draws": return drawsValue || 0
        case "losses": return lossesValue || 0
        case "goalsFor": return goalsForValue || 0
        case "goalsAgainst": return goalsAgainstValue || 0
        case "goalDifference": return goalDifferenceValue || 0
        case "points": return pointsValue || 0
        }
        return ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: metrics ? metrics.spacingMd : 16

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "Standings"
                font.pixelSize: metrics ? metrics.font(28) : 28
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Back"
                onClicked: root.backRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: metrics ? metrics.radiusMd : 8
            visible: root.hasActiveGame
            color: "#eef1f5"
            border.color: "#d2d8df"
            implicitHeight: 42

            RowLayout {
                anchors.fill: parent
                anchors.margins: root.metrics ? root.metrics.spacingSm : 8
                spacing: root.metrics ? root.metrics.spacingSm : 8

                Repeater {
                    model: root.columns()

                    delegate: Label {
                        required property var modelData
                        Layout.preferredWidth: modelData[1] > 0 ? (root.metrics ? root.metrics.px(modelData[1]) : modelData[1]) : 1
                        Layout.fillWidth: modelData[1] < 0
                        text: modelData[0]
                        font.bold: true
                        font.pixelSize: root.metrics ? root.metrics.font(13) : 13
                        color: "#203040"
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Label {
            visible: !root.hasActiveGame
            text: "No active game. Start a new game to view standings."
            color: "#666666"
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.hasActiveGame
            clip: true

            Column {
                width: root.width
                spacing: 6

                Repeater {
                    model: gameFacade.standingsModel

                    delegate: Rectangle {
                        id: rowRoot
                        readonly property var rowPosition: position
                        readonly property var rowTeamName: teamName
                        readonly property var rowPlayed: played
                        readonly property var rowWins: wins
                        readonly property var rowDraws: draws
                        readonly property var rowLosses: losses
                        readonly property var rowGoalsFor: goalsFor
                        readonly property var rowGoalsAgainst: goalsAgainst
                        readonly property var rowGoalDifference: goalDifference
                        readonly property var rowPoints: points
                        width: parent.width
                        height: root.metrics ? root.metrics.px(44) : 44
                        radius: root.metrics ? root.metrics.radiusMd : 8
                        color: isSelectedTeam ? "#e6f0ff" : "white"
                        border.color: isSelectedTeam ? "#5b8def" : "#d8d8d8"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: root.metrics ? root.metrics.spacingSm : 8
                            spacing: root.metrics ? root.metrics.spacingSm : 8

                            Repeater {
                                model: root.columns()

                                delegate: Label {
                                    required property var modelData
                                    Layout.preferredWidth: modelData[1] > 0 ? (root.metrics ? root.metrics.px(modelData[1]) : modelData[1]) : 1
                                    Layout.fillWidth: modelData[1] < 0
                                    text: root.cellText(modelData[2], rowRoot.rowPosition, rowRoot.rowTeamName, rowRoot.rowPlayed, rowRoot.rowWins, rowRoot.rowDraws, rowRoot.rowLosses, rowRoot.rowGoalsFor, rowRoot.rowGoalsAgainst, rowRoot.rowGoalDifference, rowRoot.rowPoints)
                                    font.pixelSize: root.metrics ? root.metrics.font(13) : 13
                                    font.bold: modelData[2] === "points"
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }

                Label {
                    visible: gameFacade.standingsModel.rowCount() === 0
                    text: "No data"
                    color: "#666666"
                }
            }
        }
    }
}

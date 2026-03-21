import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    modal: true
    width: 520
    height: 560
    standardButtons: Dialog.Ok
    title: playerDetails && playerDetails.hasPlayer ? (playerDetails.name || "Player") : "Player Details"

    property var playerDetails: ({})
    property var currentSeasonStats: mapValue(playerDetails, "currentSeasonStats", {})
    property var archivedSummary: mapValue(playerDetails, "archivedStatsSummary", {})

    function mapValue(mapObject, key, fallbackValue) {
        if (!mapObject || mapObject[key] === undefined || mapObject[key] === null) {
            return fallbackValue
        }
        return mapObject[key]
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 12
        clip: true

        ColumnLayout {
            width: root.width - 48
            spacing: 12

            Label {
                text: root.playerDetails && root.playerDetails.hasPlayer
                      ? (root.playerDetails.name || "Unknown")
                      : "No data"
                font.pixelSize: 24
                font.bold: true
            }

            GridLayout {
                columns: 2
                columnSpacing: 12
                rowSpacing: 8

                Label { text: "Age"; font.bold: true }
                Label { text: root.mapValue(root.playerDetails, "age", "-") }

                Label { text: "Position"; font.bold: true }
                Label { text: root.mapValue(root.playerDetails, "position", "-") }

                Label { text: "Team"; font.bold: true }
                Label { text: root.mapValue(root.playerDetails, "teamName", "-") }

                Label { text: "Overall"; font.bold: true }
                Label { text: root.mapValue(root.playerDetails, "overallSummary", "-") }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#dddddd"
            }

            Label {
                text: "Current Season"
                font.pixelSize: 18
                font.bold: true
            }

            GridLayout {
                columns: 2
                columnSpacing: 12
                rowSpacing: 8

                Label { text: "Appearances"; font.bold: true }
                Label { text: root.mapValue(root.currentSeasonStats, "appearances", 0) }

                Label { text: "Starts"; font.bold: true }
                Label { text: root.mapValue(root.currentSeasonStats, "starts", 0) }

                Label { text: "Minutes"; font.bold: true }
                Label { text: root.mapValue(root.currentSeasonStats, "minutes", 0) }

                Label { text: "Goals"; font.bold: true }
                Label { text: root.mapValue(root.currentSeasonStats, "goals", 0) }

                Label { text: "Assists"; font.bold: true }
                Label { text: root.mapValue(root.currentSeasonStats, "assists", 0) }

                Label { text: "Yellow / Red"; font.bold: true }
                Label { text: root.mapValue(root.currentSeasonStats, "yellowCards", 0) + " / " + root.mapValue(root.currentSeasonStats, "redCards", 0) }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#dddddd"
            }

            Label {
                text: "Archived Summary"
                font.pixelSize: 18
                font.bold: true
            }

            GridLayout {
                columns: 2
                columnSpacing: 12
                rowSpacing: 8

                Label { text: "Seasons"; font.bold: true }
                Label { text: root.mapValue(root.archivedSummary, "seasonCount", 0) }

                Label { text: "Appearances"; font.bold: true }
                Label { text: root.mapValue(root.archivedSummary, "totalAppearances", 0) }

                Label { text: "Goals"; font.bold: true }
                Label { text: root.mapValue(root.archivedSummary, "totalGoals", 0) }

                Label { text: "Assists"; font.bold: true }
                Label { text: root.mapValue(root.archivedSummary, "totalAssists", 0) }
            }
        }
    }
}
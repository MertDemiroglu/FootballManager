import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "UiHelpers.js" as UiHelpers

Dialog {
    id: root
    modal: true
    width: 540
    height: 600
    standardButtons: Dialog.Ok
    title: playerDetails && playerDetails.hasPlayer ? (playerDetails.name || "Player") : "Player Details"

    property var playerDetails: ({})
    property var currentSeasonStats: UiHelpers.mapValue(playerDetails, "currentSeasonStats", {})
    property var archivedSummary: UiHelpers.mapValue(playerDetails, "archivedStatsSummary", {})


    ScrollView {
        anchors.fill: parent
        anchors.margins: 12
        clip: true

        Item {
            width: Math.max(root.width - 48, 440)
            implicitHeight: contentColumn.implicitHeight

            ColumnLayout {
                id: contentColumn
                width: parent.width
                spacing: 14

                Label {
                    text: root.playerDetails && root.playerDetails.hasPlayer
                          ? (root.playerDetails.name || "Unknown")
                          : "No data"
                    font.pixelSize: 24
                    font.bold: true
                    color: "#17212f"
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 8

                    Label { text: "Age"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.playerDetails, "age", "-"); font.pixelSize: 15 }
                    Label { text: "Position"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.playerDetails, "position", "-"); font.pixelSize: 15 }
                    Label { text: "Team"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.playerDetails, "teamName", "-"); font.pixelSize: 15 }
                    Label { text: "Overall"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.playerDetails, "overallSummary", "-"); font.pixelSize: 15 }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#dddddd" }

                Label {
                    text: "Current Season"
                    font.pixelSize: 19
                    font.bold: true
                    color: "#17212f"
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 8

                    Label { text: "Appearances"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.currentSeasonStats, "appearances", 0); font.pixelSize: 15 }
                    Label { text: "Starts"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.currentSeasonStats, "starts", 0); font.pixelSize: 15 }
                    Label { text: "Minutes"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.currentSeasonStats, "minutes", 0); font.pixelSize: 15 }
                    Label { text: "Goals"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.currentSeasonStats, "goals", 0); font.pixelSize: 15 }
                    Label { text: "Assists"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.currentSeasonStats, "assists", 0); font.pixelSize: 15 }
                    Label { text: "Yellow / Red"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.currentSeasonStats, "yellowCards", 0) + " / " + UiHelpers.mapValue(root.currentSeasonStats, "redCards", 0); font.pixelSize: 15 }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#dddddd" }

                Label {
                    text: "Archived Summary"
                    font.pixelSize: 19
                    font.bold: true
                    color: "#17212f"
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 8

                    Label { text: "Seasons"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.archivedSummary, "seasonCount", 0); font.pixelSize: 15 }
                    Label { text: "Appearances"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.archivedSummary, "totalAppearances", 0); font.pixelSize: 15 }
                    Label { text: "Goals"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.archivedSummary, "totalGoals", 0); font.pixelSize: 15 }
                    Label { text: "Assists"; font.bold: true; font.pixelSize: 15 }
                    Label { text: UiHelpers.mapValue(root.archivedSummary, "totalAssists", 0); font.pixelSize: 15 }
                }
            }
        }
    }
}

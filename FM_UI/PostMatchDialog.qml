import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

InteractionModalShell {
    id: root
    property var interactionData: ({})
    signal continueRequested()
    signal viewDetailsRequested(var matchId)

    visible: false
    maxCardWidth: 980

    Label {
        width: parent.width
        text: "Match Finished"
        font.pixelSize: 24
        font.bold: true
        color: "#101828"
    }

    Rectangle {
        width: parent.width
        radius: 12
        color: "white"
        border.color: "#e4e7ec"
        implicitHeight: summaryColumn.implicitHeight + 20

        ColumnLayout {
            id: summaryColumn
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            Label {
                Layout.fillWidth: true
                text: interactionData.scoreLine
                      || ((interactionData.homeTeamName || "Home") + " "
                          + (interactionData.homeGoals !== undefined ? interactionData.homeGoals : "-")
                          + " - "
                          + (interactionData.awayGoals !== undefined ? interactionData.awayGoals : "-")
                          + " " + (interactionData.awayTeamName || "Away"))
                wrapMode: Text.WordWrap
                font.pixelSize: 20
                font.bold: true
                color: "#344054"
            }

            Label { text: "Date: " + (interactionData.dateText || "-"); color: "#667085"; font.pixelSize: 13 }
            Label { text: "Matchweek: " + (interactionData.matchweek !== undefined ? interactionData.matchweek : "-"); color: "#667085"; font.pixelSize: 13 }
            Label { text: "Scorers: " + (interactionData.scorerSummary || "No goals recorded."); color: "#475467"; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: "Assists: " + (interactionData.assistSummary || "No assists recorded."); color: "#475467"; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: "Cards: " + (interactionData.cardSummary || "No cards recorded."); color: "#475467"; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
    }

    MatchLineupSection {
        width: parent.width
        homeTeamName: interactionData.homeTeamName || "Home"
        awayTeamName: interactionData.awayTeamName || "Away"
        homeCoachName: interactionData.homeCoachName || "-"
        awayCoachName: interactionData.awayCoachName || "-"
        homeFormationText: interactionData.homeFormationText || "-"
        awayFormationText: interactionData.awayFormationText || "-"
        homeLineupRows: interactionData.homeLineup || []
        awayLineupRows: interactionData.awayLineup || []
        compactMode: true
    }

    Button {
        width: parent.width
        height: 42
        text: "Open Match Report"
        enabled: (interactionData.matchId || 0) > 0
        onClicked: root.viewDetailsRequested(interactionData.matchId || 0)
    }

    Button {
        width: parent.width
        height: 42
        text: "Continue"
        onClicked: root.continueRequested()
    }
}

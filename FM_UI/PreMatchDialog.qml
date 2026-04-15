import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

InteractionModalShell {
    id: root
    property var interactionData: ({})
    signal playMatchRequested()

    visible: false
    maxCardWidth: 980

    Label {
        width: parent.width
        text: "Pre-Match Lineup Preview"
        font.pixelSize: 24
        font.bold: true
        color: "#101828"
    }

    Rectangle {
        width: parent.width
        radius: 12
        color: "white"
        border.color: "#e4e7ec"
        implicitHeight: metaColumn.implicitHeight + 20

        ColumnLayout {
            id: metaColumn
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            Label {
                Layout.fillWidth: true
                text: interactionData.fixtureLabel
                      || ((interactionData.homeTeamName || "Home") + " vs " + (interactionData.awayTeamName || "Away"))
                font.pixelSize: 18
                font.bold: true
                color: "#344054"
                wrapMode: Text.WordWrap
            }

            Label {
                text: "Date: " + (interactionData.dateText || "-")
                font.pixelSize: 13
                color: "#667085"
            }

            Label {
                text: "Matchweek: " + (interactionData.matchweek !== undefined ? interactionData.matchweek : "-")
                font.pixelSize: 13
                color: "#667085"
            }
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
        height: 44
        text: "Play Match"
        onClicked: root.playMatchRequested()
    }
}

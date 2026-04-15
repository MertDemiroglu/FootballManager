import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

InteractionModalShell {
    id: root
    property var details: ({})
    signal closeRequested()

    visible: false
    maxCardWidth: 1020

    function clearDetails() {
        details = ({})
    }

    function openForMatch(matchId) {
        root.visible = false
        clearDetails()
        if (!matchId || matchId <= 0) {
            return
        }
        const result = gameFacade.getMatchReportDetails(matchId)
        if (!result || !result.hasData) {
            return
        }
        details = result
        root.visible = true
    }

    Rectangle {
        width: parent.width
        radius: 12
        color: "white"
        border.color: "#e4e7ec"
        implicitHeight: headerColumn.implicitHeight + 20

        ColumnLayout {
            id: headerColumn
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            Label {
                text: "Match Report"
                font.pixelSize: 24
                font.bold: true
                color: "#101828"
            }

            Label {
                text: (details.homeTeamName || "Home") + " " + (details.score || "-") + " " + (details.awayTeamName || "Away")
                font.pixelSize: 20
                font.bold: true
                color: "#344054"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label {
                text: "Date: " + (details.dateText || "-") + "   Matchweek: " + (details.matchweek !== undefined ? details.matchweek : "-")
                color: "#667085"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label { text: "Scorers: " + (details.scorers || "No goals recorded."); color: "#475467"; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: "Assists: " + (details.assists || "No assists recorded."); color: "#475467"; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: "Cards: " + (details.cards || "No cards recorded."); color: "#475467"; font.pixelSize: 13; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
    }

    MatchLineupSection {
        width: parent.width
        homeTeamName: details.homeTeamName || "Home"
        awayTeamName: details.awayTeamName || "Away"
        homeCoachName: details.homeCoachName || "-"
        awayCoachName: details.awayCoachName || "-"
        homeFormationText: details.homeFormationText || "-"
        awayFormationText: details.awayFormationText || "-"
        homeLineupRows: details.homeLineup || []
        awayLineupRows: details.awayLineup || []
        compactMode: false
    }

    Rectangle {
        width: parent.width
        radius: 12
        color: "white"
        border.color: "#e4e7ec"
        implicitHeight: 280

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Label {
                text: "Event Timeline"
                font.pixelSize: 16
                font.bold: true
                color: "#101828"
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                Column {
                    width: parent.width
                    spacing: 8

                    Repeater {
                        model: details.events || []
                        delegate: Rectangle {
                            required property var modelData
                            width: parent ? parent.width : 0
                            radius: 8
                            color: "#f8fafc"
                            border.color: "#e4e7ec"
                            implicitHeight: eventLabel.implicitHeight + 18

                            Label {
                                id: eventLabel
                                anchors.fill: parent
                                anchors.margins: 9
                                text: modelData.detailText || "Event"
                                color: "#344054"
                                font.pixelSize: 13
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    Label {
                        visible: (details.events || []).length === 0
                        width: parent.width
                        text: "No event records available."
                        color: "#667085"
                        font.pixelSize: 13
                    }
                }
            }
        }
    }

    Button {
        width: parent.width
        height: 42
        text: "Close"
        onClicked: {
            root.visible = false
            root.clearDetails()
            root.closeRequested()
        }
    }
}

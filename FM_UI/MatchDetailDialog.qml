import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

InteractionModalShell {
    id: root
    property var details: ({})
    signal closeRequested()

    visible: false
    maxCardWidth: 1020
    focus: visible
    z: 1000

    function clearDetails() {
        details = ({})
    }

    function dismiss() {
        root.visible = false
        clearDetails()
        root.closeRequested()
    }

    function fallbackDetailsForPostMatch(matchId) {
        if (!gameFacade || !gameFacade.interactionState) {
            return null
        }

        const postMatch = gameFacade.interactionState.postMatch
        if (!postMatch || !postMatch.hasData || postMatch.matchId !== matchId) {
            return null
        }

        return {
            hasData: true,
            matchId: postMatch.matchId,
            dateText: postMatch.dateText || "",
            matchweek: postMatch.matchweek !== undefined ? postMatch.matchweek : 0,
            homeTeamName: postMatch.homeTeamName || "Home",
            awayTeamName: postMatch.awayTeamName || "Away",
            score: (postMatch.homeGoals !== undefined ? postMatch.homeGoals : "-")
                   + " - "
                   + (postMatch.awayGoals !== undefined ? postMatch.awayGoals : "-"),
            scorers: postMatch.scorerSummary || "No goals recorded.",
            assists: postMatch.assistSummary || "No assists recorded.",
            cards: postMatch.cardSummary || "No cards recorded.",
            homeCoachName: postMatch.homeCoachName || "-",
            awayCoachName: postMatch.awayCoachName || "-",
            homeFormationText: postMatch.homeFormationText || "-",
            awayFormationText: postMatch.awayFormationText || "-",
            homeLineup: postMatch.homeLineup || [],
            awayLineup: postMatch.awayLineup || [],
            events: []
        }
    }

    function openForMatch(matchId) {
        root.visible = false
        clearDetails()
        if (!matchId || matchId <= 0) {
            return
        }
        const result = gameFacade ? gameFacade.getMatchReportDetails(matchId) : null
        const resolvedDetails = (result && result.hasData) ? result : fallbackDetailsForPostMatch(matchId)
        if (!resolvedDetails || !resolvedDetails.hasData) {
            return
        }
        details = resolvedDetails
        root.visible = true
    }

    onVisibleChanged: {
        if (visible) {
            root.forceActiveFocus()
        }
    }

    Keys.onEscapePressed: function(event) {
        root.dismiss()
        event.accepted = true
    }

    onOverlayClicked: root.dismiss()

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

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Label {
                    text: "Match Report"
                    font.pixelSize: 24
                    font.bold: true
                    color: "#101828"
                    Layout.fillWidth: true
                }

                Button {
                    text: "Close"
                    onClicked: root.dismiss()
                }
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
        onClicked: root.dismiss()
    }
}

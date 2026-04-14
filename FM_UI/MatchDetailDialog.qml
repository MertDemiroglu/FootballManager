import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

InteractionModalShell {
    id: root
    property var details: ({})
    signal closeRequested()

    visible: false
    maxCardWidth: 780

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

    Label {
        width: parent.width
        text: "Match Report"
        font.pixelSize: 24
        font.bold: true
        color: "#101828"
    }

    Label {
        width: parent.width
        text: (details.homeTeamName || "Home") + " " + (details.score || "-") + " " + (details.awayTeamName || "Away")
        font.pixelSize: 18
        font.bold: true
        color: "#344054"
        wrapMode: Text.WordWrap
    }

    Label {
        width: parent.width
        text: "Date: " + (details.dateText || "-") + "   Matchweek: " + (details.matchweek !== undefined ? details.matchweek : "-")
        font.pixelSize: 14
        color: "#475467"
        wrapMode: Text.WordWrap
    }

    Label {
        width: parent.width
        text: "Scorers: " + (details.scorers || "No goals recorded.")
        font.pixelSize: 14
        color: "#475467"
        wrapMode: Text.WordWrap
    }

    Label {
        width: parent.width
        text: "Assists: " + (details.assists || "No assists recorded.")
        font.pixelSize: 14
        color: "#475467"
        wrapMode: Text.WordWrap
    }

    Label {
        width: parent.width
        text: "Cards: " + (details.cards || "No cards recorded.")
        font.pixelSize: 14
        color: "#475467"
        wrapMode: Text.WordWrap
    }

    Label {
        width: parent.width
        text: "Events"
        font.pixelSize: 16
        font.bold: true
        color: "#101828"
    }

    ScrollView {
        width: parent.width
        implicitHeight: 220
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

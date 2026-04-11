import QtQuick
import QtQuick.Controls

InteractionModalShell {
    id: root
    property var interactionData: ({})
    signal continueRequested()

    visible: false

    Label {
        width: parent.width
        text: "Match Finished"
        font.pixelSize: 24
        font.bold: true
        color: "#101828"
    }

    Label {
        width: parent.width
        text: (interactionData.homeTeamName || "Home") + " "
              + (interactionData.homeGoals !== undefined ? interactionData.homeGoals : "-")
              + " - "
              + (interactionData.awayGoals !== undefined ? interactionData.awayGoals : "-")
              + " " + (interactionData.awayTeamName || "Away")
        wrapMode: Text.WordWrap
        font.pixelSize: 18
        color: "#344054"
    }

    Label {
        width: parent.width
        text: "Date: " + (interactionData.dateText || "-")
        font.pixelSize: 15
        color: "#475467"
        wrapMode: Text.WordWrap
    }

    Label {
        width: parent.width
        text: "Matchweek: " + (interactionData.matchweek !== undefined ? interactionData.matchweek : "-")
        font.pixelSize: 15
        color: "#475467"
        wrapMode: Text.WordWrap
    }

    Button {
        width: parent.width
        height: 42
        text: "Continue"
        onClicked: root.continueRequested()
    }
}

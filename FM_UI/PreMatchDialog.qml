import QtQuick
import QtQuick.Controls

InteractionModalShell {
    id: root
    property var interactionData: ({})
    signal playMatchRequested()

    visible: false

    Label {
        width: parent.width
        text: "Your match is ready"
        font.pixelSize: 24
        font.bold: true
        color: "#101828"
    }

    Label {
        width: parent.width
        text: (interactionData.homeTeamName || "Home") + " vs " + (interactionData.awayTeamName || "Away")
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
        text: "Play Match"
        onClicked: root.playMatchRequested()
    }
}

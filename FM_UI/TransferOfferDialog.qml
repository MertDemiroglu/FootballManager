import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

InteractionModalShell {
    id: root
    property var interactionData: ({})
    property real actionRowSpacing: 10
    signal acceptRequested()
    signal rejectRequested()
    signal laterRequested()

    visible: false
    maxCardWidth: 560

    Label {
        width: parent.width
        text: "Transfer Offer"
        font.pixelSize: 24
        font.bold: true
        color: "#101828"
    }

    Label {
        width: parent.width
        text: "Player: " + (interactionData.playerName || (interactionData.playerId !== undefined ? ("#" + interactionData.playerId) : "—"))
        font.pixelSize: 16
        color: "#344054"
        wrapMode: Text.WordWrap
    }

    Label {
        width: parent.width
        text: "From: " + (interactionData.sellerTeamName || "—")
        font.pixelSize: 16
        color: "#344054"
        wrapMode: Text.WordWrap
    }

    Label {
        width: parent.width
        text: "To: " + (interactionData.buyerTeamName || "—")
        font.pixelSize: 16
        color: "#344054"
        wrapMode: Text.WordWrap
    }

    Label {
        width: parent.width
        text: "Fee: " + (interactionData.fee !== undefined ? interactionData.fee : "—")
        font.pixelSize: 16
        color: "#344054"
        wrapMode: Text.WordWrap
    }

    RowLayout {
        width: parent.width
        spacing: root.actionRowSpacing

        Button {
            text: "Accept"
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            onClicked: root.acceptRequested()
        }

        Button {
            text: "Reject"
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            onClicked: root.rejectRequested()
        }

        Button {
            text: "Later"
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            onClicked: root.laterRequested()
        }
    }
}

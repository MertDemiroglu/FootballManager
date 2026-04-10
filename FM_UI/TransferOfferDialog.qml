import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var interactionData: ({})
    signal acceptRequested()
    signal rejectRequested()
    signal laterRequested()

    visible: false

    Rectangle {
        anchors.fill: parent
        color: "#66000000"
    }

    Rectangle {
        width: Math.min(parent.width - 48, 560)
        anchors.centerIn: parent
        radius: 12
        color: "#ffffff"
        border.color: "#d0d5dd"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Label {
                text: "Transfer Offer"
                font.pixelSize: 24
                font.bold: true
                color: "#101828"
            }

            Label {
                text: "Player: " + (interactionData.playerName || (interactionData.playerId !== undefined ? ("#" + interactionData.playerId) : "—"))
                font.pixelSize: 16
                color: "#344054"
                wrapMode: Text.WordWrap
            }

            Label {
                text: "From: " + (interactionData.sellerTeamName || "—")
                font.pixelSize: 16
                color: "#344054"
            }

            Label {
                text: "To: " + (interactionData.buyerTeamName || "—")
                font.pixelSize: 16
                color: "#344054"
            }

            Label {
                text: "Fee: " + (interactionData.fee !== undefined ? interactionData.fee : "—")
                font.pixelSize: 16
                color: "#344054"
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

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
    }
}
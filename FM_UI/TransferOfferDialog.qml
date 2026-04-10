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

    MouseArea {
        anchors.fill: parent
        onClicked: function(mouse) { mouse.accepted = true }
    }

    Rectangle {
        id: dialogCard
        width: Math.min(parent.width - 48, 560)
        implicitHeight: contentColumn.implicitHeight + 40
        height: implicitHeight
        anchors.centerIn: parent
        radius: 12
        color: "#ffffff"
        border.color: "#d0d5dd"

        Column {
            id: contentColumn
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.right: parent.right
            anchors.rightMargin: 20
            spacing: 12

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

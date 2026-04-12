import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    signal backRequested()

    function refreshData() {
        // no-op: data is provided by gameFacade.pendingTransferOffersModel
    }

    Component.onCompleted: refreshData()

    Connections {
        target: gameFacade

        function onGameStateChanged() {
            refreshData()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#f4f6fb"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 16

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: "Pending Transfer Offers"
                    font.pixelSize: 30
                    font.bold: true
                    color: "#17212f"
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Back"
                    onClicked: root.backRequested()
                }
            }

            Label {
                Layout.fillWidth: true
                text: "Offers deferred with Later stay here until you Accept, Reject, or they expire."
                color: "#526071"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }

            Label {
                visible: pendingOffersRepeater.count === 0
                Layout.fillWidth: true
                text: "No pending transfer offers."
                color: "#667085"
                font.pixelSize: 16
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: pendingOffersRepeater.count > 0
                clip: true

                Column {
                    width: parent.width
                    spacing: 12

                    Repeater {
                        id: pendingOffersRepeater
                        model: gameFacade.pendingTransferOffersModel

                        delegate: Rectangle {
                            width: parent.width
                            radius: 12
                            color: "#ffffff"
                            border.color: "#d8dee8"
                            implicitHeight: offerContent.implicitHeight + 24

                            ColumnLayout {
                                id: offerContent
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 6

                                Label {
                                    Layout.fillWidth: true
                                    text: (playerName || ("#" + playerId))
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "#17212f"
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: "Buyer: " + (buyerTeamName || "—")
                                    font.pixelSize: 15
                                    color: "#344054"
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: "Fee: " + (fee !== undefined ? fee : "—")
                                    font.pixelSize: 15
                                    color: "#344054"
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: "Last valid date: " + (lastDateText || "")
                                    font.pixelSize: 15
                                    color: "#344054"
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: "Offer ID: " + (offerId !== undefined ? offerId : "—")
                                    font.pixelSize: 13
                                    color: "#667085"
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Button {
                                        text: "Accept"
                                        Layout.fillWidth: true
                                        onClicked: {
                                            gameFacade.acceptTransferOfferById(offerId)
                                            root.refreshData()
                                        }
                                    }

                                    Button {
                                        text: "Reject"
                                        Layout.fillWidth: true
                                        onClicked: {
                                            gameFacade.rejectTransferOfferById(offerId)
                                            root.refreshData()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

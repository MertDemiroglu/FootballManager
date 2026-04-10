import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var interactionData: ({})
    signal continueRequested()

    visible: false

    Rectangle {
        anchors.fill: parent
        color: "#66000000"
    }

    Rectangle {
        width: Math.min(parent.width - 48, 520)
        anchors.centerIn: parent
        radius: 12
        color: "#ffffff"
        border.color: "#d0d5dd"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Label {
                text: "Match Finished"
                font.pixelSize: 24
                font.bold: true
                color: "#101828"
            }

            Label {
                text: (interactionData.homeTeamName || "Home") + " "
                      + (interactionData.homeGoals !== undefined ? interactionData.homeGoals : "—")
                      + " - "
                      + (interactionData.awayGoals !== undefined ? interactionData.awayGoals : "—")
                      + " " + (interactionData.awayTeamName || "Away")
                wrapMode: Text.WordWrap
                font.pixelSize: 18
                color: "#344054"
            }

            Label {
                text: "Date: " + (interactionData.dateText || "—")
                font.pixelSize: 15
                color: "#475467"
            }

            Label {
                text: "Matchweek: " + (interactionData.matchweek !== undefined ? interactionData.matchweek : "—")
                font.pixelSize: 15
                color: "#475467"
            }

            Item { Layout.fillHeight: true }

            Button {
                text: "Continue"
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                onClicked: root.continueRequested()
            }
        }
    }
}
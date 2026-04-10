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
        z: 0
    }

    Rectangle {
        id: dialogCard
        width: Math.min(parent.width - 48, 520)
        implicitHeight: contentLayout.implicitHeight + 40
        height: implicitHeight
        anchors.centerIn: parent
        radius: 12
        color: "#ffffff"
        border.color: "#d0d5dd"
        z: 1

        ColumnLayout {
            id: contentLayout
            width: parent.width - 40
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 20
            spacing: 12

            Label {
                text: "Match Finished"
                font.pixelSize: 24
                font.bold: true
                color: "#101828"
            }

            Label {
                text: (interactionData.homeTeamName || "Home") + " "
                      + (interactionData.homeGoals !== undefined ? interactionData.homeGoals : "-")
                      + " - "
                      + (interactionData.awayGoals !== undefined ? interactionData.awayGoals : "-")
                      + " " + (interactionData.awayTeamName || "Away")
                wrapMode: Text.WordWrap
                font.pixelSize: 18
                color: "#344054"
                Layout.fillWidth: true
            }

            Label {
                text: "Date: " + (interactionData.dateText || "-")
                font.pixelSize: 15
                color: "#475467"
                Layout.fillWidth: true
            }

            Label {
                text: "Matchweek: " + (interactionData.matchweek !== undefined ? interactionData.matchweek : "-")
                font.pixelSize: 15
                color: "#475467"
                Layout.fillWidth: true
            }

            Item { Layout.fillHeight: true }

            Button {
                text: "Continue"
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                onClicked: root.continueRequested()
            }

                   Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 0
            }
        }
    }
}
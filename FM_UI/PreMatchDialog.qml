import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var interactionData: ({})
    signal playMatchRequested()

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
        width: Math.min(parent.width - 48, 520)
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
    }
}
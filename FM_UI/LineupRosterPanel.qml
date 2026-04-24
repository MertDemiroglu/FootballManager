import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var rosterModel: null
    property int selectedPlayerId: 0

    signal playerClicked(int playerId)

    radius: 14
    border.color: "#d8dee8"
    color: "#ffffff"
    Layout.minimumHeight: 420

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Label {
            text: "Roster"
            font.pixelSize: 18
            font.bold: true
            color: "#17212f"
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                width: parent.width
                model: root.rosterModel
                spacing: 8
                boundsBehavior: Flickable.StopAtBounds

                delegate: LineupRosterRow {
                    width: ListView.view ? ListView.view.width : 0
                    selectedPlayerId: root.selectedPlayerId
                    rowData: ({
                        playerId: playerId,
                        name: name,
                        positionShort: positionShort,
                        overall: overall,
                        overallSummary: overallSummary,
                        form: form,
                        fitness: fitness,
                        morale: morale,
                        isAssigned: isAssigned,
                        assignedSlotIndex: assignedSlotIndex
                    })
                    onClicked: function(playerId) {
                        root.playerClicked(playerId)
                    }
                }
            }
        }
    }
}

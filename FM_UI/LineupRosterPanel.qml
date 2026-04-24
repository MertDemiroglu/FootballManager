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

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.rosterModel
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            delegate: LineupRosterRow {
                required property int playerId
                required property string name
                required property string positionShort
                required property int overall
                required property string overallSummary
                required property int form
                required property int fitness
                required property int morale
                required property bool isAssigned
                required property int assignedSlotIndex

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
                onClicked: function(clickedPlayerId) {
                    root.playerClicked(clickedPlayerId)
                }
            }
        }
    }
}

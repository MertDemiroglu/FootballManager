import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var slotsModel: null
    property var rosterModel: null
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1
    property int selectedPlayerId: 0

    signal slotClicked(int slotIndex)
    signal playerClicked(int playerId)

    radius: 14
    border.color: "#d8dee8"
    color: "#ffffff"
    Layout.minimumHeight: 420

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        LineupStartingXISection {
            Layout.fillWidth: true
            slotsModel: root.slotsModel
            selectedSlotIndex: root.selectedSlotIndex
            selectedSourceSlotIndex: root.selectedSourceSlotIndex
            onSlotClicked: function(slotIndex) {
                root.slotClicked(slotIndex)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: "#e4e7ec"
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: "Squad"
                font.pixelSize: 16
                font.bold: true
                color: "#17212f"
            }

            Label {
                text: (rosterModel ? rosterModel.count : 0) + " players"
                font.pixelSize: 11
                color: "#667085"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: "#f8fafc"
            border.color: "#edf2f7"
            clip: true

            ListView {
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                model: root.rosterModel
                spacing: 6
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
}

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
    readonly property var unassignedRosterRows: buildUnassignedRosterRows()
    readonly property bool isSquadDropHighlighted: squadDropArea.containsDrag

    function buildUnassignedRosterRows() {
        const rows = rosterModel && rosterModel.rows ? rosterModel.rows : []
        return rows.filter(function(row) {
            return !row.isAssigned
        })
    }

    signal slotClicked(int slotIndex)
    signal playerClicked(int playerId)
    signal playerDroppedOnSlot(int playerId, int slotIndex)
    signal slotDroppedOnSlot(int sourceSlotIndex, int targetSlotIndex)
    signal playerDroppedOnSquad(int playerId, int sourceSlotIndex)

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
            onPlayerDroppedOnSlot: function(playerId, slotIndex) {
                root.playerDroppedOnSlot(playerId, slotIndex)
            }
            onSlotDroppedOnSlot: function(sourceSlotIndex, targetSlotIndex) {
                root.slotDroppedOnSlot(sourceSlotIndex, targetSlotIndex)
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
                text: unassignedRosterRows.length + " available"
                font.pixelSize: 11
                color: "#667085"
            }
        }

        Rectangle {
            id: squadSection
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: root.isSquadDropHighlighted ? "#ecfeff" : "#f8fafc"
            border.color: root.isSquadDropHighlighted ? "#06b6d4" : "#edf2f7"
            border.width: root.isSquadDropHighlighted ? 2 : 1
            clip: true

            DropArea {
                id: squadDropArea
                anchors.fill: parent
                z: 20
                keys: [ "lineup-player", "lineup-slot" ]

                onDropped: function(drop) {
                    const source = drop.source
                    if (!source)
                        return

                    if (source.dragKind === "slot" && source.dragAssignedPlayerId > 0) {
                        root.playerDroppedOnSquad(source.dragAssignedPlayerId, source.dragSourceSlotIndex)
                        drop.acceptProposedAction()
                    } else if (source.dragKind === "player" && source.dragPlayerId > 0 && source.dragSourceSlotIndex >= 0) {
                        root.playerDroppedOnSquad(source.dragPlayerId, source.dragSourceSlotIndex)
                        drop.acceptProposedAction()
                    }
                }
            }

            ListView {
                id: squadList
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.topMargin: 6
                anchors.rightMargin: 18
                anchors.bottomMargin: 6
                clip: true
                model: root.unassignedRosterRows
                spacing: 6
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                delegate: LineupRosterRow {
                    width: ListView.view ? ListView.view.width : 0
                    selectedPlayerId: root.selectedPlayerId
                    rowData: modelData
                    onClicked: function(clickedPlayerId) {
                        root.playerClicked(clickedPlayerId)
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: squadList.count === 0
                text: "No available squad players"
                font.pixelSize: 12
                color: "#667085"
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // GameFacade/backend remains the single source of truth for lineup editor data.
    property var gameFacade
    property bool readOnly: false
    property var lineupState: gameFacade ? gameFacade.editableLineupState : null
    property var slotsModel: gameFacade ? gameFacade.editableLineupSlotsModel : null
    property var rosterModel: gameFacade ? gameFacade.editableLineupRosterModel : null
    property int selectedSlotIndex: -1
    property int selectedPlayerId: 0
    property string actionStatusText: ""

    function selectSlot(slotIndex) {
        selectedSlotIndex = slotIndex
        actionStatusText = ""
    }

    function selectPlayer(playerId) {
        selectedPlayerId = playerId
        actionStatusText = ""
    }

    function assignSelectedPlayerToSelectedSlot() {
        if (!gameFacade || selectedSlotIndex < 0 || selectedPlayerId <= 0)
            return

        const ok = gameFacade.assignEditableLineupPlayerToSlot(selectedPlayerId, selectedSlotIndex)
        if (ok) {
            actionStatusText = "Assigned player to slot."
            selectedPlayerId = 0
        } else {
            actionStatusText = "Assign failed."
        }
    }

    function clearSelectedSlot() {
        if (!gameFacade || selectedSlotIndex < 0)
            return

        const ok = gameFacade.clearEditableLineupSlot(selectedSlotIndex)
        if (ok) {
            actionStatusText = "Slot cleared."
            selectedPlayerId = 0
        } else {
            actionStatusText = "Clear slot failed."
        }
    }

    function unassignSelectedPlayer() {
        if (!gameFacade || selectedPlayerId <= 0)
            return

        const ok = gameFacade.unassignEditableLineupPlayer(selectedPlayerId)
        if (ok) {
            actionStatusText = "Player unassigned."
            selectedPlayerId = 0
        } else {
            actionStatusText = "Unassign failed."
        }
    }

    implicitHeight: layoutRoot.implicitHeight

    ColumnLayout {
        id: layoutRoot
        width: parent ? parent.width : 0
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: lineupState && lineupState.hasLineup
                  ? "Formation " + (lineupState.formationText || "-") + " • Assigned " + (lineupState.assignedCount || 0) + "/" + (lineupState.slotCount || 0)
                  : "Lineup data unavailable"
            font.pixelSize: 14
            color: "#526071"
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 10
            color: "#f8fafc"
            border.color: "#d0d5dd"
            implicitHeight: actionBarContent.implicitHeight + 16

            ColumnLayout {
                id: actionBarContent
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: selectedSlotIndex >= 0 ? ("Selected Slot: #" + selectedSlotIndex) : "Selected Slot: None"
                        font.pixelSize: 12
                        color: "#344054"
                    }

                    Label {
                        text: selectedPlayerId > 0 ? ("Selected Player ID: " + selectedPlayerId) : "Selected Player: None"
                        font.pixelSize: 12
                        color: "#344054"
                    }

                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        text: "Assign"
                        enabled: selectedSlotIndex >= 0 && selectedPlayerId > 0
                        onClicked: root.assignSelectedPlayerToSelectedSlot()
                    }

                    Button {
                        text: "Clear Slot"
                        enabled: selectedSlotIndex >= 0
                        onClicked: root.clearSelectedSlot()
                    }

                    Button {
                        text: "Unassign Player"
                        enabled: selectedPlayerId > 0
                        onClicked: root.unassignSelectedPlayer()
                    }

                    Item { Layout.fillWidth: true }
                }

                Label {
                    Layout.fillWidth: true
                    text: actionStatusText
                    visible: actionStatusText.length > 0
                    color: "#475467"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            LineupPitchPanel {
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                slotsModel: root.slotsModel
                selectedSlotIndex: root.selectedSlotIndex
                onSlotClicked: function(slotIndex) {
                    root.selectSlot(slotIndex)
                }
            }

            LineupRosterPanel {
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                rosterModel: root.rosterModel
                selectedPlayerId: root.selectedPlayerId
                onPlayerClicked: function(playerId) {
                    root.selectPlayer(playerId)
                }
            }
        }
    }
}

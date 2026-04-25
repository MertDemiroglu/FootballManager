import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    implicitHeight: Math.max(620, layoutRoot.implicitHeight)

    // Uses the global GameFacade context property; backend models remain the source of truth.
    readonly property var lineupState: gameFacade.editableLineupState
    readonly property var slotsModel: gameFacade.editableLineupSlotsModel
    readonly property var rosterModel: gameFacade.editableLineupRosterModel
    readonly property bool hasValidLineupData: lineupState
        && lineupState.hasLineup
        && slotsModel
        && slotsModel.count > 0
        && rosterModel
        && rosterModel.count > 0
        && gameFacade.getSelectedTeamId() > 0
        && gameFacade.getSelectedLeagueId() > 0
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1
    property int selectedPlayerId: 0
    property string actionStatusText: ""
    property var supportedFormations: []
    property bool isSyncingFormationSelection: false

    function refreshSupportedFormations() {
        supportedFormations = gameFacade ? gameFacade.getEditableLineupSupportedFormations() : []
        syncFormationSelection()
    }

    function formationIndexFor(formationId) {
        for (let i = 0; i < supportedFormations.length; ++i) {
            if (supportedFormations[i].formationId === formationId)
                return i
        }
        return -1
    }

    function syncFormationSelection() {
        if (!formationSelector || !lineupState || !lineupState.hasLineup)
            return

        isSyncingFormationSelection = true
        formationSelector.currentIndex = formationIndexFor(lineupState.formation)
        isSyncingFormationSelection = false
    }

    function changeFormationFromSelector(index) {
        if (isSyncingFormationSelection || !gameFacade || !lineupState || !lineupState.hasLineup)
            return
        if (index < 0 || index >= supportedFormations.length)
            return

        const formationId = supportedFormations[index].formationId
        if (formationId === undefined || formationId < 0 || formationId === lineupState.formation)
            return

        const ok = gameFacade.changeEditableLineupFormation(formationId)
        if (ok) {
            selectedSlotIndex = -1
            selectedSourceSlotIndex = -1
            selectedPlayerId = 0
            actionStatusText = "Formation changed. Lineup cleared."
            syncFormationSelection()
        } else {
            actionStatusText = "Formation change failed."
            syncFormationSelection()
        }
    }

    function selectSlot(slotIndex) {
        selectedSlotIndex = slotIndex
        if (selectedSourceSlotIndex === slotIndex)
            selectedSourceSlotIndex = -1
        actionStatusText = ""
    }

    function selectPlayer(playerId) {
        selectedPlayerId = playerId
        actionStatusText = ""
    }

    function setSelectedSlotAsSource() {
        if (selectedSlotIndex < 0)
            return

        selectedSourceSlotIndex = selectedSlotIndex
        actionStatusText = ""
    }

    function slotRowFor(slotIndex) {
        const rows = slotsModel ? slotsModel.rows : []
        for (let i = 0; i < rows.length; ++i) {
            if (rows[i].slotIndex === slotIndex)
                return rows[i]
        }
        return null
    }

    function rosterRowFor(playerId) {
        const rows = rosterModel ? rosterModel.rows : []
        for (let i = 0; i < rows.length; ++i) {
            if (rows[i].playerId === playerId)
                return rows[i]
        }
        return null
    }

    function slotLabelFor(slotIndex) {
        const row = slotRowFor(slotIndex)
        if (!row)
            return slotIndex >= 0 ? ("#" + slotIndex) : "None"
        return "#" + row.slotIndex + " " + (row.slotLabel || row.slotRole || "Slot")
    }

    function playerLabelFor(playerId) {
        const row = rosterRowFor(playerId)
        if (!row)
            return playerId > 0 ? ("ID " + playerId) : "None"
        return row.name + " (" + (row.positionShort || "?") + ")"
    }

    function selectedPlayerCurrentSlotText() {
        const row = rosterRowFor(selectedPlayerId)
        if (!row)
            return "Selected Player Current Slot: None"
        return row.isAssigned
            ? ("Selected Player Current Slot: " + slotLabelFor(row.assignedSlotIndex) + " - assigning elsewhere will move them")
            : "Selected Player Current Slot: Unassigned"
    }

    function targetSlotOccupantText() {
        const row = slotRowFor(selectedSlotIndex)
        if (!row)
            return "Target Slot Occupant: None"
        return row.hasAssignedPlayer
            ? ("Target Slot Occupant: " + (row.assignedPlayerName || "Unknown") + " - assigning another player will replace them")
            : "Target Slot Occupant: Empty"
    }

    function assignSelectedPlayerToSelectedSlot() {
        if (!gameFacade || selectedSlotIndex < 0 || selectedPlayerId <= 0)
            return

        const playerRow = rosterRowFor(selectedPlayerId)
        const targetRow = slotRowFor(selectedSlotIndex)
        const ok = gameFacade.assignEditableLineupPlayerToSlot(selectedPlayerId, selectedSlotIndex)
        if (ok) {
            if (playerRow && playerRow.isAssigned && playerRow.assignedSlotIndex !== selectedSlotIndex)
                actionStatusText = "Moved player to selected slot."
            else if (targetRow && targetRow.hasAssignedPlayer && targetRow.assignedPlayerId !== selectedPlayerId)
                actionStatusText = "Replaced target slot occupant."
            else
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

    function swapSelectedSlots() {
        if (!gameFacade || selectedSourceSlotIndex < 0 || selectedSlotIndex < 0 || selectedSourceSlotIndex === selectedSlotIndex)
            return

        const sourceRow = slotRowFor(selectedSourceSlotIndex)
        const targetRow = slotRowFor(selectedSlotIndex)
        const ok = gameFacade.swapEditableLineupSlots(selectedSourceSlotIndex, selectedSlotIndex)
        if (ok) {
            actionStatusText = sourceRow && sourceRow.hasAssignedPlayer && targetRow && targetRow.hasAssignedPlayer
                ? "Swapped selected slots."
                : "Moved selected slot occupant."
            selectedSourceSlotIndex = -1
        } else {
            actionStatusText = "Swap / move slots failed."
        }
    }

    function autoSelectLineup() {
        if (!gameFacade)
            return

        const ok = gameFacade.autoSelectEditableLineup()
        if (ok) {
            selectedSlotIndex = -1
            selectedSourceSlotIndex = -1
            selectedPlayerId = 0
            actionStatusText = "Auto selected lineup."
        } else {
            actionStatusText = "Auto select failed."
        }
    }

    function handlePlayerDroppedOnSlot(playerId, targetSlotIndex) {
        if (!gameFacade || playerId <= 0 || targetSlotIndex < 0)
            return

        const ok = gameFacade.assignEditableLineupPlayerToSlot(playerId, targetSlotIndex)
        if (ok) {
            selectedPlayerId = 0
            selectedSourceSlotIndex = -1
            selectedSlotIndex = targetSlotIndex
            actionStatusText = "Dropped player into slot."
        } else {
            actionStatusText = "Drop assign failed."
        }
    }

    function handleSlotDroppedOnSlot(sourceSlotIndex, targetSlotIndex) {
        if (!gameFacade || sourceSlotIndex < 0 || targetSlotIndex < 0)
            return
        if (sourceSlotIndex === targetSlotIndex) {
            selectedSourceSlotIndex = -1
            selectedSlotIndex = targetSlotIndex
            actionStatusText = ""
            return
        }

        const ok = gameFacade.swapEditableLineupSlots(sourceSlotIndex, targetSlotIndex)
        if (ok) {
            selectedSourceSlotIndex = -1
            selectedPlayerId = 0
            selectedSlotIndex = targetSlotIndex
            actionStatusText = "Dropped slot into slot."
        } else {
            actionStatusText = "Drop swap failed."
        }
    }

    function handlePlayerDroppedOnSquad(playerId, sourceSlotIndex) {
        if (!gameFacade || playerId <= 0 || sourceSlotIndex < 0)
            return

        const ok = gameFacade.unassignEditableLineupPlayer(playerId)
        if (ok) {
            if (selectedPlayerId === playerId)
                selectedPlayerId = 0
            if (selectedSourceSlotIndex === sourceSlotIndex)
                selectedSourceSlotIndex = -1
            if (selectedSlotIndex === sourceSlotIndex)
                selectedSlotIndex = -1
            actionStatusText = "Player moved back to squad."
        } else {
            actionStatusText = "Drop unassign failed."
        }
    }

    Component.onCompleted: refreshSupportedFormations()

    Connections {
        target: lineupState
        function onChanged() {
            root.syncFormationSelection()
        }
    }

    ColumnLayout {
        id: layoutRoot
        anchors.fill: parent
        spacing: 10

        Label {
            Layout.fillWidth: true
            text: lineupState && lineupState.hasLineup
                  ? "Formation " + (lineupState.formationText || "-") + " - Assigned " + (lineupState.assignedCount || 0) + "/" + (lineupState.slotCount || 0)
                  : "Lineup data unavailable"
            font.pixelSize: 14
            color: "#526071"
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 8
            color: "#fbfcfe"
            border.color: "#e4e7ec"
            implicitHeight: actionBarContent.implicitHeight + 12

            ColumnLayout {
                id: actionBarContent
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "Formation"
                        font.pixelSize: 12
                        color: "#344054"
                    }

                    ComboBox {
                        id: formationSelector
                        Layout.preferredWidth: 120
                        model: root.supportedFormations
                        textRole: "formationText"
                        valueRole: "formationId"
                        enabled: root.hasValidLineupData && count > 0
                        onActivated: function(index) {
                            root.changeFormationFromSelector(index)
                        }
                    }

                    Label {
                        text: "Selected Slot: " + slotLabelFor(selectedSlotIndex)
                        font.pixelSize: 12
                        color: "#344054"
                    }

                    Label {
                        text: "Source Slot: " + slotLabelFor(selectedSourceSlotIndex)
                        font.pixelSize: 12
                        color: "#344054"
                    }

                    Label {
                        text: "Selected Player: " + playerLabelFor(selectedPlayerId)
                        font.pixelSize: 12
                        color: "#344054"
                    }

                    Item { Layout.fillWidth: true }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: width < 760 ? 2 : 6
                    rowSpacing: 6
                    columnSpacing: 6

                    Label {
                        Layout.fillWidth: true
                        Layout.columnSpan: width < 760 ? 2 : 6
                        text: "Manual controls"
                        font.pixelSize: 11
                        font.bold: true
                        color: "#667085"
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Auto Select"
                        enabled: root.hasValidLineupData
                        onClicked: root.autoSelectLineup()
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Assign / Move Player To Slot"
                        enabled: selectedSlotIndex >= 0 && selectedPlayerId > 0
                        onClicked: root.assignSelectedPlayerToSelectedSlot()
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Use Selected Slot As Source"
                        enabled: selectedSlotIndex >= 0
                        onClicked: root.setSelectedSlotAsSource()
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Clear Slot"
                        enabled: selectedSlotIndex >= 0
                        onClicked: root.clearSelectedSlot()
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Unassign Player"
                        enabled: selectedPlayerId > 0
                        onClicked: root.unassignSelectedPlayer()
                    }

                    Button {
                        Layout.fillWidth: true
                        text: "Swap / Move Slots"
                        enabled: selectedSourceSlotIndex >= 0
                            && selectedSlotIndex >= 0
                            && selectedSourceSlotIndex !== selectedSlotIndex
                        onClicked: root.swapSelectedSlots()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: actionStatusText.length > 0 ? actionStatusText
                        : selectedPlayerCurrentSlotText() + "  /  " + targetSlotOccupantText()
                    color: "#475467"
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !root.hasValidLineupData
            color: "#b42318"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            text: "Lineup debug: hasLineup=" + (lineupState ? lineupState.hasLineup : false)
                + ", slots=" + (slotsModel ? slotsModel.count : 0)
                + ", roster=" + (rosterModel ? rosterModel.count : 0)
                + ", selectedTeamId=" + (gameFacade ? gameFacade.getSelectedTeamId() : 0)
                + ", selectedLeagueId=" + (gameFacade ? gameFacade.getSelectedLeagueId() : 0)
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 420
            spacing: 12

            LineupPitchPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 3
                slotsModel: root.slotsModel
                selectedSlotIndex: root.selectedSlotIndex
                selectedSourceSlotIndex: root.selectedSourceSlotIndex
                onSlotClicked: function(slotIndex) {
                    root.selectSlot(slotIndex)
                }
                onPlayerDroppedOnSlot: function(playerId, slotIndex) {
                    root.handlePlayerDroppedOnSlot(playerId, slotIndex)
                }
                onSlotDroppedOnSlot: function(sourceSlotIndex, targetSlotIndex) {
                    root.handleSlotDroppedOnSlot(sourceSlotIndex, targetSlotIndex)
                }
                onPlayerDroppedOnSquad: function(playerId, sourceSlotIndex) {
                    root.handlePlayerDroppedOnSquad(playerId, sourceSlotIndex)
                }
            }

            LineupRosterPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 2
                slotsModel: root.slotsModel
                rosterModel: root.rosterModel
                selectedSlotIndex: root.selectedSlotIndex
                selectedSourceSlotIndex: root.selectedSourceSlotIndex
                selectedPlayerId: root.selectedPlayerId
                onSlotClicked: function(slotIndex) {
                    root.selectSlot(slotIndex)
                }
                onPlayerClicked: function(playerId) {
                    root.selectPlayer(playerId)
                }
                onPlayerDroppedOnSlot: function(playerId, slotIndex) {
                    root.handlePlayerDroppedOnSlot(playerId, slotIndex)
                }
                onSlotDroppedOnSlot: function(sourceSlotIndex, targetSlotIndex) {
                    root.handleSlotDroppedOnSlot(sourceSlotIndex, targetSlotIndex)
                }
            }
        }
    }
}

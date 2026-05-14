import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true
    implicitHeight: Math.max(700, layoutRoot.implicitHeight)

    // Uses the global GameFacade context property; backend models remain the source of truth.
    readonly property var lineupState: gameFacade.editableLineupState
    readonly property var slotsModel: gameFacade.editableLineupSlotsModel
    readonly property var rosterModel: gameFacade.editableLineupRosterModel
    readonly property var substitutesModel: gameFacade.editableLineupSubstitutesModel
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
    property var supportedMentalities: []
    property var supportedTempos: []
    property bool isSyncingFormationSelection: false
    property string selectedMentality: "Balanced"
    property string selectedMentalityCode: "balanced"
    property string selectedTempo: "Normal"
    property string selectedTempoCode: "normal"

    readonly property color panelColor: "#0f1a24"
    readonly property color borderColor: "#263847"
    readonly property color textPrimary: "#f7fbff"
    readonly property color textSecondary: "#91a4b6"
    readonly property color accentGreen: "#20b765"
    readonly property color accentAmber: "#f5b942"

    function refreshSupportedFormations() {
        supportedFormations = gameFacade ? gameFacade.getEditableLineupSupportedFormations() : []
        syncFormationSelection()
    }

    function refreshTacticalOptions() {
        supportedMentalities = gameFacade ? gameFacade.getEditableLineupSupportedMentalities() : []
        supportedTempos = gameFacade ? gameFacade.getEditableLineupSupportedTempos() : []
        syncTacticalSetup()
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

    function syncTacticalSetup() {
        if (!gameFacade)
            return

        const tacticalSetup = gameFacade.getEditableLineupTacticalSetup()
        if (!tacticalSetup)
            return

        selectedMentalityCode = tacticalSetup.mentalityCode || "balanced"
        selectedMentality = tacticalSetup.mentalityText || "Balanced"
        selectedTempoCode = tacticalSetup.tempoCode || "normal"
        selectedTempo = tacticalSetup.tempoText || "Normal"
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

    function applyLineupToMatch() {
        if (!gameFacade)
            return

        const ok = gameFacade.applyEditableLineupToActivePreMatch()
        actionStatusText = ok
            ? "Lineup applied to match."
            : "Lineup incomplete. Current pre-match lineup unchanged."
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

    function assignedSummaryText() {
        if (!lineupState || !lineupState.hasLineup)
            return "Lineup data unavailable"
        return "Formation " + (lineupState.formationText || "-")
            + "  -  Assigned " + (lineupState.assignedCount || 0)
            + "/" + (lineupState.slotCount || 0)
    }

    function isLineupFull() {
        return lineupState && lineupState.hasLineup
            && (lineupState.assignedCount || 0) >= (lineupState.slotCount || 0)
            && (lineupState.slotCount || 0) > 0
    }

    Component.onCompleted: {
        refreshSupportedFormations()
        refreshTacticalOptions()
    }

    Connections {
        target: lineupState
        function onChanged() {
            root.syncFormationSelection()
            root.syncTacticalSetup()
        }
    }

    ColumnLayout {
        id: layoutRoot
        anchors.fill: parent
        spacing: 10

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: "Lineup Editor"
                font.pixelSize: 30
                font.bold: true
                color: root.textPrimary
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    text: root.lineupState && root.lineupState.hasLineup
                          ? ("Formation " + (root.lineupState.formationText || "-"))
                          : "Lineup data unavailable"
                    font.pixelSize: 14
                    color: "#c7d1db"
                }

                Rectangle {
                    Layout.preferredWidth: 7
                    Layout.preferredHeight: 7
                    radius: 4
                    color: root.isLineupFull() ? root.accentGreen : root.accentAmber
                }

                Label {
                    Layout.fillWidth: true
                    text: root.lineupState && root.lineupState.hasLineup
                          ? ("Assigned " + (root.lineupState.assignedCount || 0) + "/" + (root.lineupState.slotCount || 0))
                          : ""
                    font.pixelSize: 14
                    color: "#c7d1db"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Button {
                    text: "Auto Select"
                    enabled: root.hasValidLineupData
                    Layout.preferredWidth: 146
                    Layout.preferredHeight: 40
                    onClicked: root.autoSelectLineup()
                    contentItem: Label {
                        text: parent.text
                        color: "#04130b"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 8
                        color: parent.enabled ? (parent.down ? "#18a356" : root.accentGreen) : "#263442"
                        border.color: parent.enabled ? "#5ee08f" : "#3a4a58"
                    }
                }

                Button {
                    text: "Use For Match"
                    visible: gameFacade
                             && gameFacade.interactionState.hasActiveInteraction
                             && gameFacade.interactionState.kind === "pre_match"
                    enabled: root.hasValidLineupData
                    Layout.preferredWidth: 132
                    Layout.preferredHeight: 40
                    onClicked: root.applyLineupToMatch()
                    contentItem: Label {
                        text: parent.text
                        color: root.textPrimary
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 8
                        color: parent.enabled ? (parent.down ? "#1d2b38" : "#111c28") : "#263442"
                        border.color: parent.enabled ? (parent.hovered ? "#4c657a" : "#33485a") : "#3a4a58"
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: actionStatusText
                    visible: actionStatusText.length > 0
                    font.pixelSize: 12
                    color: "#90f0b8"
                    elide: Text.ElideRight
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !root.hasValidLineupData
            color: "#f87171"
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
            Layout.minimumHeight: 500
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 3
                spacing: 12

                LineupPitchPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
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
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 8
                    color: "#0d1721"
                    border.color: root.borderColor
                    implicitHeight: tacticalContent.implicitHeight + 24

                    RowLayout {
                        id: tacticalContent
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 16

                        ColumnLayout {
                            Layout.preferredWidth: 158
                            spacing: 10

                            Label {
                                text: "Formation"
                                font.pixelSize: 13
                                color: "#c7d1db"
                            }

                            ComboBox {
                                id: formationSelector
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                model: root.supportedFormations
                                textRole: "formationText"
                                valueRole: "formationId"
                                enabled: root.hasValidLineupData && count > 0
                                onActivated: function(index) {
                                    root.changeFormationFromSelector(index)
                                }

                                contentItem: Label {
                                    leftPadding: 14
                                    rightPadding: 30
                                    text: formationSelector.displayText
                                    color: root.textPrimary
                                    font.pixelSize: 14
                                    font.bold: true
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                                indicator: Label {
                                    x: formationSelector.width - width - 14
                                    y: (formationSelector.height - height) / 2
                                    text: "v"
                                    color: "#c7d1db"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                background: Rectangle {
                                    radius: 7
                                    color: "#111c28"
                                    border.color: formationSelector.enabled ? "#33485a" : "#263442"
                                }
                                popup: Popup {
                                    y: formationSelector.height + 4
                                    width: formationSelector.width
                                    implicitHeight: contentItem.implicitHeight + 8
                                    padding: 4
                                    background: Rectangle {
                                        radius: 7
                                        color: "#101a25"
                                        border.color: "#33485a"
                                    }
                                    contentItem: ListView {
                                        implicitHeight: contentHeight
                                        model: formationSelector.popup.visible ? formationSelector.delegateModel : null
                                        currentIndex: formationSelector.highlightedIndex
                                        clip: true
                                    }
                                }
                                delegate: ItemDelegate {
                                    width: formationSelector.width - 8
                                    height: 32
                                    contentItem: Label {
                                        text: modelData.formationText || ""
                                        color: "#f7fbff"
                                        font.pixelSize: 13
                                        verticalAlignment: Text.AlignVCenter
                                        leftPadding: 8
                                    }
                                    background: Rectangle {
                                        radius: 5
                                        color: highlighted ? "#183524" : "transparent"
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Label {
                                text: "Mentality"
                                font.pixelSize: 13
                                color: "#c7d1db"
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Repeater {
                                    model: root.supportedMentalities.length > 0
                                        ? root.supportedMentalities
                                        : [ { stableCode: "defensive", displayText: "Defensive" },
                                            { stableCode: "balanced", displayText: "Balanced" },
                                            { stableCode: "attacking", displayText: "Attacking" } ]
                                    delegate: Button {
                                        required property var modelData
                                        text: modelData.displayText || ""
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        onClicked: {
                                            const code = modelData.stableCode || ""
                                            const ok = gameFacade ? gameFacade.setEditableLineupMentality(code) : false
                                            if (ok) {
                                                root.selectedMentalityCode = code
                                                root.selectedMentality = modelData.displayText || root.selectedMentality
                                            } else {
                                                root.actionStatusText = "Mentality change failed."
                                            }
                                        }
                                        contentItem: Label {
                                            text: parent.text
                                            color: root.selectedMentalityCode === parent.modelData.stableCode ? "#f7fbff" : "#c7d1db"
                                            font.pixelSize: 13
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        background: Rectangle {
                                            radius: 0
                                            color: root.selectedMentalityCode === parent.modelData.stableCode ? "#105e34" : "#111c28"
                                            border.color: root.selectedMentalityCode === parent.modelData.stableCode ? "#2fb565" : "#33485a"
                                        }
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Label {
                                text: "Tempo"
                                font.pixelSize: 13
                                color: "#c7d1db"
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Repeater {
                                    model: root.supportedTempos.length > 0
                                        ? root.supportedTempos
                                        : [ { stableCode: "low", displayText: "Low" },
                                            { stableCode: "normal", displayText: "Normal" },
                                            { stableCode: "high", displayText: "High" } ]
                                    delegate: Button {
                                        required property var modelData
                                        text: modelData.displayText || ""
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        onClicked: {
                                            const code = modelData.stableCode || ""
                                            const ok = gameFacade ? gameFacade.setEditableLineupTempo(code) : false
                                            if (ok) {
                                                root.selectedTempoCode = code
                                                root.selectedTempo = modelData.displayText || root.selectedTempo
                                            } else {
                                                root.actionStatusText = "Tempo change failed."
                                            }
                                        }
                                        contentItem: Label {
                                            text: parent.text
                                            color: root.selectedTempoCode === parent.modelData.stableCode ? "#f7fbff" : "#c7d1db"
                                            font.pixelSize: 13
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        background: Rectangle {
                                            radius: 0
                                            color: root.selectedTempoCode === parent.modelData.stableCode ? "#105e34" : "#111c28"
                                            border.color: root.selectedTempoCode === parent.modelData.stableCode ? "#2fb565" : "#33485a"
                                        }
                                    }
                                }
                            }
                        }

                        Button {
                            text: "More Options"
                            Layout.preferredWidth: 132
                            Layout.preferredHeight: 38
                            Layout.alignment: Qt.AlignBottom
                            onClicked: root.actionStatusText = "More tactical options coming soon."
                            contentItem: Label {
                                text: parent.text
                                color: root.textPrimary
                                font.pixelSize: 13
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                radius: 7
                                color: parent.down ? "#1d2b38" : "#111c28"
                                border.color: parent.hovered ? "#4c657a" : "#33485a"
                            }
                        }

                        Label {
                            Layout.preferredWidth: 1
                            visible: false
                            text: selectedPlayerCurrentSlotText() + targetSlotOccupantText()
                        }
                    }
                }
            }

            LineupRosterPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 2
                slotsModel: root.slotsModel
                rosterModel: root.rosterModel
                substitutesModel: root.substitutesModel
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
                onPlayerDroppedOnSquad: function(playerId, sourceSlotIndex) {
                    root.handlePlayerDroppedOnSquad(playerId, sourceSlotIndex)
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    property var slotsModel: null
    property var rosterModel: null
    property var substitutesModel: null
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1
    property int selectedPlayerId: 0
    property string squadFilter: "All"
    property var metrics: null
    readonly property int metricColumnWidth: metrics ? metrics.px(metrics.dense ? 48 : 58) : 58
    readonly property int scrollbarGutter: metrics ? metrics.px(16) : 16
    readonly property int metricHeaderRightInset: metrics ? metrics.px(metrics.dense ? 18 : 30) : 30
    readonly property int topCardMargin: metrics ? metrics.spacingMd : 14
    readonly property var assignedPlayerIdMap: buildAssignedPlayerIdMap()
    readonly property var unassignedRosterRows: buildUnassignedRosterRows()
    readonly property var filteredRosterRows: buildFilteredRosterRows()
    readonly property bool isSquadDropHighlighted: squadDropArea.containsDrag

    function normalizedPlayerId(value) {
        const id = Number(value || 0)
        return isNaN(id) ? 0 : id
    }

    function markAssignedPlayer(map, value) {
        const playerId = normalizedPlayerId(value)
        if (playerId > 0)
            map[playerId] = true
    }

    function buildAssignedPlayerIdMap() {
        const assigned = ({})
        const slotRows = slotsModel && slotsModel.rows ? slotsModel.rows : []
        for (let i = 0; i < slotRows.length; ++i)
            markAssignedPlayer(assigned, slotRows[i].assignedPlayerId)

        const substituteRows = substitutesModel && substitutesModel.rows ? substitutesModel.rows : []
        for (let j = 0; j < substituteRows.length; ++j)
            markAssignedPlayer(assigned, substituteRows[j].playerId)

        return assigned
    }

    function isPlayerAssignedAnywhere(playerId) {
        const id = normalizedPlayerId(playerId)
        return id > 0 && assignedPlayerIdMap[id] === true
    }

    function buildUnassignedRosterRows() {
        const rows = rosterModel && rosterModel.rows ? rosterModel.rows : []
        return rows.filter(function(row) {
            return !root.isPlayerAssignedAnywhere(row.playerId)
        })
    }

    function positionGroup(positionText) {
        const position = (positionText || "").toUpperCase()
        if (position === "GK")
            return "GK"
        if (position === "CB" || position === "LB" || position === "RB" || position === "LWB" || position === "RWB")
            return "DEF"
        if (position === "DM" || position === "CM" || position === "AM")
            return "MID"
        if (position === "LM" || position === "RM" || position === "LW" || position === "RW" || position === "ST" || position === "CF" || position === "FW")
            return "ATT"
        return "All"
    }

    function buildFilteredRosterRows() {
        if (squadFilter === "All")
            return unassignedRosterRows
        return unassignedRosterRows.filter(function(row) {
            return positionGroup(row.positionShort) === squadFilter
        })
    }

    signal slotClicked(int slotIndex)
    signal playerClicked(int playerId)
    signal playerDroppedOnSlot(int playerId, int slotIndex)
    signal slotDroppedOnSlot(int sourceSlotIndex, int targetSlotIndex)
    signal playerDroppedOnSquad(int playerId, int sourceSlotIndex)

    spacing: metrics ? metrics.spacingSm : 8

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: startingContent.implicitHeight + root.topCardMargin * 2
        radius: metrics ? metrics.radiusMd : 8
        color: "#0d1721"
        border.color: "#263847"
        clip: true

        LineupStartingXISection {
            id: startingContent
            anchors.fill: parent
            anchors.margins: root.topCardMargin
            metrics: root.metrics
            slotsModel: root.slotsModel
            substitutesModel: root.substitutesModel
            selectedSlotIndex: root.selectedSlotIndex
            selectedSourceSlotIndex: root.selectedSourceSlotIndex
            metricColumnWidth: root.metricColumnWidth
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
    }

    Rectangle {
        id: squadCard
        Layout.fillWidth: true
        Layout.fillHeight: true
        radius: metrics ? metrics.radiusMd : 8
        color: "#0d1721"
        border.color: root.isSquadDropHighlighted ? "#23c7d4" : "#263847"
        border.width: root.isSquadDropHighlighted ? 2 : 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.metrics ? root.metrics.spacingMd : 14
            spacing: root.metrics ? root.metrics.spacingSm : 8

            RowLayout {
                Layout.fillWidth: true
                Layout.rightMargin: root.metricHeaderRightInset
                spacing: 8

                ComboBox {
                    id: squadFilterSelector
                    Layout.preferredWidth: root.metrics ? root.metrics.px(root.metrics.dense ? 126 : 150) : 150
                    Layout.preferredHeight: root.metrics ? root.metrics.px(root.metrics.dense ? 30 : 34) : 34
                    model: [ "All", "GK", "DEF", "MID", "ATT" ]
                    currentIndex: Math.max(0, [ "All", "GK", "DEF", "MID", "ATT" ].indexOf(root.squadFilter))
                    onActivated: function(index) {
                        root.squadFilter = squadFilterSelector.model[index]
                    }

                    contentItem: Label {
                        leftPadding: 12
                        rightPadding: 24
                        text: "Filter: " + squadFilterSelector.currentText
                        color: "#f7fbff"
                        font.pixelSize: root.metrics ? root.metrics.font(13) : 13
                        font.bold: true
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    indicator: Label {
                        x: squadFilterSelector.width - width - 10
                        y: (squadFilterSelector.height - height) / 2
                        text: "v"
                        color: "#c7d1db"
                        font.pixelSize: root.metrics ? root.metrics.font(12) : 12
                        font.bold: true
                    }
                    background: Rectangle {
                        radius: 7
                        color: "#111c28"
                        border.color: squadFilterSelector.hovered ? "#4c657a" : "#33485a"
                    }
                    popup: Popup {
                        y: squadFilterSelector.height + 4
                        width: squadFilterSelector.width
                        implicitHeight: contentItem.implicitHeight + 8
                        padding: 4
                        background: Rectangle {
                            radius: 7
                            color: "#101a25"
                            border.color: "#33485a"
                        }
                        contentItem: ListView {
                            implicitHeight: contentHeight
                            model: squadFilterSelector.popup.visible ? squadFilterSelector.delegateModel : null
                            currentIndex: squadFilterSelector.highlightedIndex
                            clip: true
                        }
                    }
                    delegate: ItemDelegate {
                        width: squadFilterSelector.width - 8
                        height: 30
                        contentItem: Label {
                            text: modelData
                            color: "#f7fbff"
                            font.pixelSize: root.metrics ? root.metrics.font(12) : 12
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 8
                        }
                        background: Rectangle {
                            radius: 5
                            color: highlighted ? "#183524" : "transparent"
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Label { Layout.preferredWidth: root.metricColumnWidth; text: root.metrics && root.metrics.dense ? "OVR" : "Overall"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                Label { Layout.preferredWidth: root.metricColumnWidth; text: "Form"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                Label { Layout.preferredWidth: root.metricColumnWidth; text: root.metrics && root.metrics.dense ? "Fit" : "Fitness"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                Label { Layout.preferredWidth: root.metricColumnWidth; text: "Moral"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Rectangle {
                id: squadSection
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: root.isSquadDropHighlighted ? "#123642" : "#0b1520"
                border.color: root.isSquadDropHighlighted ? "#23c7d4" : "#1f3040"
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
                    anchors.rightMargin: 6
                    anchors.bottomMargin: 6
                    clip: true
                    model: root.filteredRosterRows
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {
                        id: squadScrollBar
                        policy: ScrollBar.AsNeeded
                        width: 5
                        anchors.right: parent.right
                        contentItem: Rectangle {
                            implicitWidth: 5
                            radius: 3
                            color: squadScrollBar.pressed || squadScrollBar.hovered ? "#6f8396" : "#566a7c"
                        }
                        background: Rectangle {
                            radius: 3
                            color: "#0b1520"
                            opacity: 0.85
                        }
                    }

                    delegate: LineupRosterRow {
                        width: ListView.view ? ListView.view.width - root.scrollbarGutter - 4 : 0
                        selectedPlayerId: root.selectedPlayerId
                        rowData: modelData
                        metricColumnWidth: root.metricColumnWidth
                        metrics: root.metrics
                        onClicked: function(clickedPlayerId) {
                            root.playerClicked(clickedPlayerId)
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: squadList.count === 0
                    text: root.squadFilter === "All" ? "No squad players shown" : "No players in this filter"
                    font.pixelSize: root.metrics ? root.metrics.font(12) : 12
                    color: "#91a4b6"
                }
            }
        }
    }
}

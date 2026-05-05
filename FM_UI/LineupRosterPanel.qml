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
    property string squadFilter: "All"
    readonly property int metricColumnWidth: 54
    readonly property var unassignedRosterRows: buildUnassignedRosterRows()
    readonly property var filteredRosterRows: buildFilteredRosterRows()
    readonly property bool isSquadDropHighlighted: squadDropArea.containsDrag

    function buildUnassignedRosterRows() {
        const rows = rosterModel && rosterModel.rows ? rosterModel.rows : []
        return rows.filter(function(row) {
            return !row.isAssigned
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

    function squadCountText() {
        if (squadFilter === "All")
            return unassignedRosterRows.length + " available"
        return filteredRosterRows.length + " shown / " + unassignedRosterRows.length + " available"
    }

    signal slotClicked(int slotIndex)
    signal playerClicked(int playerId)
    signal playerDroppedOnSlot(int playerId, int slotIndex)
    signal slotDroppedOnSlot(int sourceSlotIndex, int targetSlotIndex)
    signal playerDroppedOnSquad(int playerId, int sourceSlotIndex)

    radius: 14
    border.color: "#263847"
    color: "#0f1a24"
    Layout.minimumHeight: 420

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

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
            color: "#253747"
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: "Squad"
                font.pixelSize: 16
                font.bold: true
                color: "#f7fbff"
            }

            Label {
                text: root.squadCountText()
                font.pixelSize: 11
                color: "#91a4b6"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Repeater {
                model: [ "All", "GK", "DEF", "MID", "ATT" ]
                delegate: Button {
                    required property string modelData
                    text: modelData
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: modelData === "All" ? 54 : 48
                    onClicked: root.squadFilter = modelData
                    contentItem: Label {
                        text: parent.text
                        color: root.squadFilter === parent.text ? "#06120b" : "#91a4b6"
                        font.pixelSize: 11
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 7
                        color: root.squadFilter === parent.text ? "#20b765" : "#162432"
                        border.color: root.squadFilter === parent.text ? "#63e69a" : "#2b4052"
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 18
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: "Player"
                font.pixelSize: 10
                font.bold: true
                color: "#6f8498"
            }

            Label { Layout.preferredWidth: root.metricColumnWidth; text: "Overall"; font.pixelSize: 10; font.bold: true; color: "#6f8498"; horizontalAlignment: Text.AlignHCenter }
            Label { Layout.preferredWidth: root.metricColumnWidth; text: "Form"; font.pixelSize: 10; font.bold: true; color: "#6f8498"; horizontalAlignment: Text.AlignHCenter }
            Label { Layout.preferredWidth: root.metricColumnWidth; text: "Fitness"; font.pixelSize: 10; font.bold: true; color: "#6f8498"; horizontalAlignment: Text.AlignHCenter }
            Label { Layout.preferredWidth: root.metricColumnWidth; text: "Moral"; font.pixelSize: 10; font.bold: true; color: "#6f8498"; horizontalAlignment: Text.AlignHCenter }
        }

        Rectangle {
            id: squadSection
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: root.isSquadDropHighlighted ? "#123642" : "#0b1520"
            border.color: root.isSquadDropHighlighted ? "#23c7d4" : "#263847"
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
                model: root.filteredRosterRows
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                delegate: LineupRosterRow {
                    width: ListView.view ? ListView.view.width : 0
                    selectedPlayerId: root.selectedPlayerId
                    rowData: modelData
                    metricColumnWidth: root.metricColumnWidth
                    onClicked: function(clickedPlayerId) {
                        root.playerClicked(clickedPlayerId)
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: squadList.count === 0
                text: root.squadFilter === "All" ? "No available squad players" : "No players in this filter"
                font.pixelSize: 12
                color: "#91a4b6"
            }
        }
    }
}

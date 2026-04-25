import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var slotData: ({})
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1

    readonly property int slotIndex: typeof slotData.slotIndex === "number" ? slotData.slotIndex : -1
    readonly property bool hasPlayer: !!slotData.hasAssignedPlayer && !slotData.isEmpty
    readonly property bool isSelected: selectedSlotIndex >= 0 && slotIndex === selectedSlotIndex
    readonly property bool isSourceSelected: selectedSourceSlotIndex >= 0 && slotIndex === selectedSourceSlotIndex
    readonly property bool isDropHighlighted: slotDropArea.containsDrag

    signal clicked(int slotIndex)
    signal playerDroppedOnSlot(int playerId, int slotIndex)
    signal slotDroppedOnSlot(int sourceSlotIndex, int targetSlotIndex)

    radius: 8
    color: isDropHighlighted ? "#ecfeff" : (isSelected ? "#dbeafe" : (isSourceSelected ? "#dcfce7" : (hasPlayer ? "#ffffff" : "#fffbeb")))
    border.color: isDropHighlighted ? "#06b6d4" : (isSelected ? "#2563eb" : (isSourceSelected ? "#16a34a" : (hasPlayer ? "#e2e8f0" : "#f59e0b")))
    border.width: isDropHighlighted || isSelected || isSourceSelected ? 2 : 1
    opacity: slotDragArea.drag.active ? 0.72 : 1.0
    implicitHeight: 34

    Item {
        id: slotDragSource
        width: root.width
        height: root.height
        x: 0
        y: 0

        property string dragKind: "slot"
        property int dragSourceSlotIndex: root.slotIndex
        property int dragPlayerId: root.slotData.assignedPlayerId || 0
        property int dragAssignedPlayerId: root.slotData.assignedPlayerId || 0

        Drag.active: root.hasPlayer && slotDragArea.drag.active
        Drag.keys: [ "lineup-slot" ]
        Drag.hotSpot.x: width / 2
        Drag.hotSpot.y: height / 2
    }

    DropArea {
        id: slotDropArea
        anchors.fill: parent
        keys: [ "lineup-player", "lineup-slot" ]

        onDropped: function(drop) {
            const source = drop.source
            if (!source || root.slotIndex < 0)
                return

            if (source.dragKind === "player") {
                root.playerDroppedOnSlot(source.dragPlayerId || 0, root.slotIndex)
                drop.acceptProposedAction()
            } else if (source.dragKind === "slot") {
                const sourceSlotIndex = source.dragSourceSlotIndex
                if (sourceSlotIndex !== root.slotIndex)
                    root.slotDroppedOnSlot(sourceSlotIndex, root.slotIndex)
                drop.acceptProposedAction()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 8

        PositionBadge {
            text: root.slotData.slotLabel || root.slotData.slotRole || "?"
            roleKey: root.slotData.slotRoleKey || -1
            implicitHeight: 22
        }

        Label {
            Layout.fillWidth: true
            text: root.hasPlayer ? (root.slotData.assignedPlayerName || "Unknown") : "Empty"
            font.pixelSize: 12
            font.bold: root.hasPlayer
            color: root.hasPlayer ? "#101828" : "#92400e"
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        Label {
            text: root.hasPlayer
                  ? (root.slotData.assignedPlayerOverallSummary || ("OVR " + (root.slotData.assignedPlayerOverall || "-")))
                  : "-"
            font.pixelSize: 11
            font.bold: root.hasPlayer
            color: root.hasPlayer ? "#475467" : "#b45309"
        }
    }

    MouseArea {
        id: slotDragArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        drag.target: root.hasPlayer ? slotDragSource : null
        drag.threshold: 8
        onReleased: {
            if (root.hasPlayer) {
                slotDragSource.Drag.drop()
                slotDragSource.x = 0
                slotDragSource.y = 0
            }
        }
        onClicked: {
            if (root.slotIndex >= 0)
                root.clicked(root.slotIndex)
        }
    }
}

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property var slotData: ({})
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1
    readonly property int slotIndex: typeof slotData.slotIndex === "number" ? slotData.slotIndex : -1
    readonly property bool isSelected: selectedSlotIndex >= 0 && slotIndex === selectedSlotIndex
    readonly property bool isSourceSelected: selectedSourceSlotIndex >= 0 && slotIndex === selectedSourceSlotIndex
    readonly property bool hasPlayer: !!slotData.hasAssignedPlayer && !slotData.isEmpty
    readonly property bool isDropHighlighted: slotDropArea.containsDrag
    property real dragHotSpotX: width / 2
    property real dragHotSpotY: height / 2

    signal clicked(int slotIndex)
    signal playerDroppedOnSlot(int playerId, int slotIndex)
    signal slotDroppedOnSlot(int sourceSlotIndex, int targetSlotIndex)

    radius: 8
    color: isDropHighlighted ? "#ecfeff" : (isSelected ? "#dbeafe" : (isSourceSelected ? "#dcfce7" : (hasPlayer ? "#f8fafc" : "#fffbeb")))
    border.color: isDropHighlighted ? "#06b6d4" : (isSelected ? "#2563eb" : (isSourceSelected ? "#16a34a" : (hasPlayer ? "#e2e8f0" : "#f59e0b")))
    border.width: isDropHighlighted || isSelected || isSourceSelected ? 2 : 1
    opacity: slotDragArea.drag.active ? 0.72 : 1.0
    implicitWidth: 132
    implicitHeight: 60

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
        Drag.hotSpot.x: root.dragHotSpotX
        Drag.hotSpot.y: root.dragHotSpotY
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

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 6
        radius: 3
        color: root.isSelected ? "#2563eb" : (root.isSourceSelected ? "#16a34a" : (root.hasPlayer ? "#64748b" : "#f59e0b"))
        opacity: 0.95
    }

    Column {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 8
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        spacing: 1

        Label {
            width: parent.width
            text: root.slotData.slotLabel || root.slotData.slotRole || "Slot"
            font.pixelSize: 10
            font.bold: true
            color: root.isSelected ? "#1d4ed8" : (root.isSourceSelected ? "#15803d" : "#475569")
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }

        Label {
            width: parent.width
            text: root.hasPlayer ? (root.slotData.assignedPlayerName || "Unknown") : "Empty"
            font.pixelSize: 12
            font.bold: root.hasPlayer
            color: root.hasPlayer ? "#0f172a" : "#92400e"
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }

        Label {
            width: parent.width
            text: root.hasPlayer
                  ? (root.slotData.assignedPlayerOverallSummary || ("OVR " + (root.slotData.assignedPlayerOverall || "-")))
                  : "-"
            font.pixelSize: 10
            color: root.hasPlayer ? "#64748b" : "#b45309"
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }
    }

    MouseArea {
        id: slotDragArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        drag.target: root.hasPlayer ? slotDragSource : null
        drag.threshold: 8
        onPressed: function(mouse) {
            if (root.hasPlayer) {
                root.dragHotSpotX = mouse.x
                root.dragHotSpotY = mouse.y
            }
        }
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

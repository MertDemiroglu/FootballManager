import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property var slotData: ({})
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1
    property string warningText: ""
    property string warningLevel: "none"
    readonly property int slotIndex: typeof slotData.slotIndex === "number" ? slotData.slotIndex : -1
    readonly property bool isSelected: selectedSlotIndex >= 0 && slotIndex === selectedSlotIndex
    readonly property bool isSourceSelected: selectedSourceSlotIndex >= 0 && slotIndex === selectedSourceSlotIndex
    readonly property bool hasPlayer: !!slotData.hasAssignedPlayer && !slotData.isEmpty
    readonly property bool isDropHighlighted: slotDropArea.containsDrag
    readonly property bool hasWarning: warningText.length > 0 && warningLevel !== "none"
    property string kitColorPrimary: "#f97316"
    property string kitColorSecondary: "#22c55e"
    property real dragHotSpotX: width / 2
    property real dragHotSpotY: height / 2

    signal clicked(int slotIndex)
    signal playerDroppedOnSlot(int playerId, int slotIndex)
    signal slotDroppedOnSlot(int sourceSlotIndex, int targetSlotIndex)

    radius: 12
    color: isDropHighlighted ? "#123642" : (isSelected ? "#10291d" : "transparent")
    border.color: isDropHighlighted
                  ? "#23c7d4"
                  : (isSourceSelected ? "#f59e0b"
                                      : (isSelected ? "#22c55e"
                                                    : (hasWarning ? "#b45309" : "transparent")))
    border.width: (isDropHighlighted || isSelected || isSourceSelected || hasWarning) ? 2 : 1
    opacity: slotDragArea.drag.active ? 0.72 : 1.0
    implicitWidth: 108
    implicitHeight: 110

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

    LineupPlayerToken {
        anchors.centerIn: parent
        positionText: root.slotData.slotLabel || root.slotData.slotRole || "Slot"
        playerName: root.slotData.assignedPlayerName || ""
        kitColorPrimary: root.kitColorPrimary
        kitColorSecondary: root.kitColorSecondary
        metricText: root.hasPlayer && root.slotData.assignedPlayerOverall > 0
                    ? String(root.slotData.assignedPlayerOverall)
                    : ""
        showMetric: root.hasPlayer
        mode: "lineupEditor"
        empty: !root.hasPlayer
    }

    Label {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 4
        anchors.rightMargin: 6
        visible: root.hasWarning
        text: "!"
        font.pixelSize: 12
        font.bold: true
        color: "#b45309"
        ToolTip.visible: warningMouse.containsMouse
        ToolTip.text: root.warningText

        MouseArea {
            id: warningMouse
            anchors.fill: parent
            hoverEnabled: true
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

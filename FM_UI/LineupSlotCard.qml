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
    property real dragHotSpotX: width / 2
    property real dragHotSpotY: height / 2

    signal clicked(int slotIndex)
    signal playerDroppedOnSlot(int playerId, int slotIndex)
    signal slotDroppedOnSlot(int sourceSlotIndex, int targetSlotIndex)

    radius: 8
    color: isDropHighlighted ? "#123642" : (hasPlayer ? "#111c28" : "#161820")
    border.color: isDropHighlighted ? "#23c7d4" : (hasPlayer ? "#34495c" : "#f5b942")
    border.width: isDropHighlighted ? 2 : 1
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
        anchors.leftMargin: 6
        anchors.topMargin: 6
        width: roleLabel.implicitWidth + 12
        height: 18
        radius: 5
        color: root.hasPlayer ? "#1a2b3a" : "#312515"
        border.color: root.hasPlayer ? "#40566a" : "#f5b942"
        opacity: 0.95

        Label {
            id: roleLabel
            anchors.centerIn: parent
            text: root.slotData.slotLabel || root.slotData.slotRole || "Slot"
            font.pixelSize: 9
            font.bold: true
            color: root.hasPlayer ? "#dce7f0" : "#ffd77a"
        }
    }

    Column {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.topMargin: 27
        anchors.bottomMargin: 6
        spacing: 1

        Label {
            width: parent.width
            text: root.hasPlayer ? (root.slotData.assignedPlayerName || "Unknown") : "Empty"
            font.pixelSize: 12
            font.bold: root.hasPlayer
            color: root.hasPlayer ? "#f7fbff" : "#f5b942"
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }

        Rectangle {
            width: parent.width
            height: 18
            radius: 5
            color: root.hasPlayer ? "#263746" : "#211a12"
            border.color: root.hasPlayer ? "#3a4d5e" : "#8a641e"

            Label {
                anchors.centerIn: parent
                text: root.hasPlayer
                      ? (root.slotData.assignedPlayerOverallSummary || ("OVR " + (root.slotData.assignedPlayerOverall || "-")))
                      : "OVR -"
                font.pixelSize: 10
                font.bold: true
                color: root.hasPlayer ? "#c9d6e2" : "#f5b942"
                elide: Text.ElideRight
            }
        }
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

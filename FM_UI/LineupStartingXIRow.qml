import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var slotData: ({})
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1
    property string warningText: ""
    property string warningLevel: "none"
    property int metricColumnWidth: 54
    property var metrics: null
    readonly property real scaleFactor: metrics ? metrics.scale : 1.0

    readonly property int slotIndex: typeof slotData.slotIndex === "number" ? slotData.slotIndex : -1
    readonly property bool hasPlayer: !!slotData.hasAssignedPlayer && !slotData.isEmpty
    readonly property bool isSelected: selectedSlotIndex >= 0 && slotIndex === selectedSlotIndex
    readonly property bool isSourceSelected: selectedSourceSlotIndex >= 0 && slotIndex === selectedSourceSlotIndex
    readonly property bool isDropHighlighted: slotDropArea.containsDrag
    readonly property bool hasWarning: warningText.length > 0 && warningLevel !== "none"
    property real dragHotSpotX: width / 2
    property real dragHotSpotY: height / 2

    signal clicked(int slotIndex)
    signal playerDroppedOnSlot(int playerId, int slotIndex)
    signal slotDroppedOnSlot(int sourceSlotIndex, int targetSlotIndex)

    function numberOnly(summary, fallbackValue) {
        const text = summary ? String(summary) : String(fallbackValue || "-")
        return text.replace(/^OVR\s+/i, "")
    }

    radius: metrics ? metrics.radiusMd : 8
    color: isDropHighlighted ? "#123642" : (hasPlayer ? "#101a25" : "#151820")
    border.color: isDropHighlighted ? "#23c7d4" : (hasPlayer ? "#253747" : "#f5b942")
    border.width: isDropHighlighted ? 2 : 1
    opacity: slotDragArea.drag.active ? 0.72 : 1.0
    implicitHeight: Math.round(42 * scaleFactor)

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

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.metrics ? root.metrics.spacingSm : 8
        anchors.rightMargin: root.metrics ? root.metrics.spacingSm : 8
        spacing: root.metrics ? root.metrics.spacingSm : 8

        PositionBadge {
            text: root.slotData.slotLabel || root.slotData.slotRole || "?"
            roleKey: root.slotData.slotRoleKey || -1
            implicitHeight: Math.round(22 * root.scaleFactor)
            metrics: root.metrics
        }

        Label {
            Layout.fillWidth: true
            text: root.hasPlayer ? (root.slotData.assignedPlayerName || "Unknown") : "Empty"
            font.pixelSize: root.metrics ? root.metrics.font(12) : 12
            font.bold: root.hasPlayer
            color: root.hasPlayer ? "#f7fbff" : "#f5b942"
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            Layout.preferredWidth: root.metricColumnWidth
            Layout.preferredHeight: Math.round(22 * root.scaleFactor)
            radius: 999
            color: "#233241"
            border.color: "#3a4d5e"

            Label {
                anchors.centerIn: parent
                text: root.hasPlayer
                      ? root.numberOnly(root.slotData.assignedPlayerOverallSummary, root.slotData.assignedPlayerOverall)
                      : "-"
                font.pixelSize: root.metrics ? root.metrics.font(10) : 10
                font.bold: true
                color: root.hasPlayer ? "#d7e0e8" : "#7d8d9a"
            }
        }

        ConditionBadge {
            label: "F"
            value: root.hasPlayer ? (root.slotData.assignedPlayerForm || 0) : 0
            compact: true
            valueOnly: true
            Layout.preferredWidth: root.metricColumnWidth
            metrics: root.metrics
        }

        ConditionBadge {
            label: "Fit"
            value: root.hasPlayer ? (root.slotData.assignedPlayerFitness || 0) : 0
            compact: true
            valueOnly: true
            Layout.preferredWidth: root.metricColumnWidth
            metrics: root.metrics
        }

        ConditionBadge {
            label: "M"
            value: root.hasPlayer ? (root.slotData.assignedPlayerMorale || 0) : 0
            compact: true
            valueOnly: true
            Layout.preferredWidth: root.metricColumnWidth
            metrics: root.metrics
        }

    }

    Label {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 4
        anchors.topMargin: 3
        visible: root.hasWarning
        text: "!"
        font.pixelSize: root.metrics ? root.metrics.font(12) : 12
        font.bold: true
        color: "#f5b942"
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

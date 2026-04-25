import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var rowData: ({})
    property int selectedPlayerId: 0
    readonly property int playerId: rowData.playerId || 0
    readonly property bool isSelected: playerId > 0 && playerId === selectedPlayerId
    readonly property int sourceSlotIndex: rowData.isAssigned ? (rowData.assignedSlotIndex || -1) : -1
    property real dragHotSpotX: width / 2
    property real dragHotSpotY: height / 2

    signal clicked(int playerId)

    radius: 8
    border.color: isSelected ? "#2563eb" : "#e4e7ec"
    border.width: isSelected ? 2 : 1
    color: isSelected ? "#dbeafe" : "#ffffff"
    opacity: playerDragArea.drag.active ? 0.72 : 1.0
    implicitHeight: 50

    Item {
        id: playerDragSource
        width: root.width
        height: root.height
        x: 0
        y: 0

        property string dragKind: "player"
        property int dragPlayerId: root.playerId
        property int dragSourceSlotIndex: root.sourceSlotIndex
        property int dragAssignedPlayerId: root.playerId

        Drag.active: root.playerId > 0 && playerDragArea.drag.active
        Drag.keys: [ "lineup-player" ]
        Drag.hotSpot.x: root.dragHotSpotX
        Drag.hotSpot.y: root.dragHotSpotY
    }

    RowLayout {
        id: rosterContent
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 8

        PositionBadge {
            text: rowData.positionShort || "?"
            implicitHeight: 24
        }

        Label {
            text: rowData.overallSummary || ("OVR " + (rowData.overall || "-"))
            font.pixelSize: 11
            font.bold: true
            color: "#475467"
        }

        Label {
            Layout.fillWidth: true
            text: rowData.name || "Unknown"
            font.pixelSize: 13
            font.bold: true
            color: "#101828"
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        ConditionBadge {
            label: "F"
            value: rowData.form
            compact: true
        }

        ConditionBadge {
            label: "Fit"
            value: rowData.fitness
            compact: true
        }

        ConditionBadge {
            label: "M"
            value: rowData.morale
            compact: true
        }
    }

    MouseArea {
        id: playerDragArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        drag.target: root.playerId > 0 ? playerDragSource : null
        drag.threshold: 8
        onPressed: function(mouse) {
            if (root.playerId > 0) {
                root.dragHotSpotX = mouse.x
                root.dragHotSpotY = mouse.y
            }
        }
        onReleased: {
            if (root.playerId > 0) {
                playerDragSource.Drag.drop()
                playerDragSource.x = 0
                playerDragSource.y = 0
            }
        }
        onClicked: {
            if (root.playerId > 0) {
                root.clicked(root.playerId)
            }
        }
    }
}

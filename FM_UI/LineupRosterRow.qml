import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var rowData: ({})
    property int selectedPlayerId: 0
    property int metricColumnWidth: 54
    readonly property int playerId: rowData.playerId || 0
    readonly property bool isSelected: playerId > 0 && playerId === selectedPlayerId
    readonly property int sourceSlotIndex: rowData.isAssigned ? (rowData.assignedSlotIndex || -1) : -1
    property real dragHotSpotX: width / 2
    property real dragHotSpotY: height / 2

    signal clicked(int playerId)

    radius: 8
    border.color: isSelected ? "#4a90ff" : "#253747"
    border.width: isSelected ? 2 : 1
    color: isSelected ? "#132f55" : "#101a25"
    opacity: playerDragArea.drag.active ? 0.72 : 1.0
    implicitHeight: 42

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
            Layout.fillWidth: true
            text: rowData.name || "Unknown"
            font.pixelSize: 12
            font.bold: true
            color: "#f7fbff"
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            Layout.preferredWidth: root.metricColumnWidth
            Layout.preferredHeight: 22
            radius: 999
            color: "#233241"
            border.color: "#3a4d5e"

            Label {
                anchors.centerIn: parent
                text: rowData.overallSummary || ("OVR " + (rowData.overall || "-"))
                font.pixelSize: 10
                font.bold: true
                color: "#d7e0e8"
            }
        }

        ConditionBadge {
            label: "F"
            value: rowData.form
            compact: true
            Layout.preferredWidth: root.metricColumnWidth
        }

        ConditionBadge {
            label: "Fit"
            value: rowData.fitness
            compact: true
            Layout.preferredWidth: root.metricColumnWidth
        }

        ConditionBadge {
            label: "M"
            value: rowData.morale
            compact: true
            Layout.preferredWidth: root.metricColumnWidth
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

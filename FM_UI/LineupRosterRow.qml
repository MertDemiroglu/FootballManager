import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var rowData: ({})
    property int selectedPlayerId: 0
    property int metricColumnWidth: 54
    property var metrics: null
    property string sourceKind: "squad"
    readonly property real scaleFactor: metrics ? metrics.visualScale : 1.0
    readonly property int playerId: rowData.playerId || 0
    readonly property bool isSelected: playerId > 0 && playerId === selectedPlayerId
    readonly property int sourceSlotIndex: rowData.isAssigned ? (rowData.assignedSlotIndex || -1) : -1
    property real dragHotSpotX: width / 2
    property real dragHotSpotY: height / 2

    signal clicked(int playerId)

    function numberOnly(summary, fallbackValue) {
        const text = summary ? String(summary) : String(fallbackValue || "-")
        return text.replace(/^OVR\s+/i, "")
    }

    radius: metrics ? metrics.radiusMd : 8
    border.color: "#253747"
    border.width: 1
    color: "#101a25"
    opacity: playerDragArea.drag.active ? 0.72 : 1.0
    implicitHeight: Math.round(42 * scaleFactor)

    Item {
        id: playerDragSource
        width: root.width
        height: root.height
        x: 0
        y: 0

        property string dragKind: "player"
        property string sourceKind: root.sourceKind
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
        anchors.leftMargin: root.metrics ? root.metrics.spacingSm : 8
        anchors.rightMargin: root.metrics ? root.metrics.spacingSm : 8
        spacing: root.metrics ? root.metrics.spacingSm : 8

        PositionBadge {
            text: rowData.positionShort || "?"
            implicitHeight: Math.round(24 * root.scaleFactor)
            metrics: root.metrics
        }

        Label {
            Layout.fillWidth: true
            text: rowData.name || "Unknown"
            font.pixelSize: root.metrics ? root.metrics.font(12) : 12
            font.bold: true
            color: "#f7fbff"
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
                text: root.numberOnly(rowData.overallSummary, rowData.overall)
                font.pixelSize: root.metrics ? root.metrics.font(10) : 10
                font.bold: true
                color: "#d7e0e8"
            }
        }

        ConditionBadge {
            label: "F"
            value: rowData.form
            compact: true
            valueOnly: true
            Layout.preferredWidth: root.metricColumnWidth
            metrics: root.metrics
        }

        ConditionBadge {
            label: "Fit"
            value: rowData.fitness
            compact: true
            valueOnly: true
            Layout.preferredWidth: root.metricColumnWidth
            metrics: root.metrics
        }

        ConditionBadge {
            label: "M"
            value: rowData.morale
            compact: true
            valueOnly: true
            Layout.preferredWidth: root.metricColumnWidth
            metrics: root.metrics
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

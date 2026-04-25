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

    signal clicked(int slotIndex)

    radius: 8
    color: isSelected ? "#dbeafe" : (isSourceSelected ? "#dcfce7" : (hasPlayer ? "#f8fafc" : "#fffbeb"))
    border.color: isSelected ? "#2563eb" : (isSourceSelected ? "#16a34a" : (hasPlayer ? "#e2e8f0" : "#f59e0b"))
    border.width: isSelected || isSourceSelected ? 2 : 1
    implicitWidth: 132
    implicitHeight: 60

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
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (root.slotIndex >= 0)
                root.clicked(root.slotIndex)
        }
    }
}

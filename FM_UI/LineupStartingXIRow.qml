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

    signal clicked(int slotIndex)

    radius: 8
    color: isSelected ? "#dbeafe" : (isSourceSelected ? "#dcfce7" : (hasPlayer ? "#ffffff" : "#fffbeb"))
    border.color: isSelected ? "#2563eb" : (isSourceSelected ? "#16a34a" : (hasPlayer ? "#e2e8f0" : "#f59e0b"))
    border.width: isSelected || isSourceSelected ? 2 : 1
    implicitHeight: 34

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
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (root.slotIndex >= 0)
                root.clicked(root.slotIndex)
        }
    }
}

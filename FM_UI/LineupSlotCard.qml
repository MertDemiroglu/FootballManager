import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var slotData: ({})
    property int selectedSlotIndex: -1
    property bool interactive: true
    readonly property int slotIndex: typeof slotData.slotIndex === "number" ? slotData.slotIndex : -1
    readonly property bool isSelected: selectedSlotIndex >= 0 && slotIndex === selectedSlotIndex

    signal clicked(int slotIndex)

    radius: 8
    color: isSelected ? "#dbeafe" : (slotData.isEmpty ? "#fef3c7" : "#f8fafc")
    border.color: isSelected ? "#2563eb" : (slotData.isEmpty ? "#f59e0b" : "#d0d5dd")
    border.width: isSelected ? 2 : 1
    implicitWidth: 96
    implicitHeight: 76

    Column {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 2

        Label {
            width: parent.width
            text: slotData.slotLabel || slotData.slotRole || "Slot"
            font.pixelSize: 11
            font.bold: true
            color: "#344054"
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }

        Label {
            width: parent.width
            text: slotData.isEmpty ? "Empty" : (slotData.assignedPlayerName || "Unknown")
            font.pixelSize: 11
            font.bold: !slotData.isEmpty
            color: slotData.isEmpty ? "#92400e" : "#101828"
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }

        Label {
            width: parent.width
            text: slotData.isEmpty ? "—" : (slotData.assignedPlayerOverallSummary || "OVR -")
            font.pixelSize: 10
            color: "#475467"
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.interactive
        cursorShape: root.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (root.slotIndex >= 0) {
                root.clicked(root.slotIndex)
            }
        }
    }
}

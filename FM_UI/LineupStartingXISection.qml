import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    property var slotsModel: null
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1
    property int metricColumnWidth: 58

    signal slotClicked(int slotIndex)
    signal playerDroppedOnSlot(int playerId, int slotIndex)
    signal slotDroppedOnSlot(int sourceSlotIndex, int targetSlotIndex)

    function orderedSlots() {
        const rows = slotsModel && slotsModel.rows ? slotsModel.rows.slice() : []
        rows.sort(function(lhs, rhs) {
            const leftOrder = typeof lhs.displayOrder === "number" ? lhs.displayOrder : lhs.slotIndex
            const rightOrder = typeof rhs.displayOrder === "number" ? rhs.displayOrder : rhs.slotIndex
            if (leftOrder !== rightOrder)
                return leftOrder - rightOrder
            return (lhs.slotIndex || 0) - (rhs.slotIndex || 0)
        })
        return rows
    }

    spacing: 8

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: "Starting XI"
            font.pixelSize: 16
            font.bold: true
            color: "#f7fbff"
        }

        Label {
            text: (slotsModel ? slotsModel.count : 0) + " slots"
            font.pixelSize: 11
            color: "#91a4b6"
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: "Player"
            font.pixelSize: 10
            font.bold: true
            color: "#6f8498"
        }

        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Overall"; font.pixelSize: 10; font.bold: true; color: "#6f8498"; horizontalAlignment: Text.AlignHCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Form"; font.pixelSize: 10; font.bold: true; color: "#6f8498"; horizontalAlignment: Text.AlignHCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Fitness"; font.pixelSize: 10; font.bold: true; color: "#6f8498"; horizontalAlignment: Text.AlignHCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Moral"; font.pixelSize: 10; font.bold: true; color: "#6f8498"; horizontalAlignment: Text.AlignHCenter }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Repeater {
            model: root.orderedSlots()

            delegate: LineupStartingXIRow {
                Layout.fillWidth: true
                slotData: modelData
                selectedSlotIndex: root.selectedSlotIndex
                selectedSourceSlotIndex: root.selectedSourceSlotIndex
                metricColumnWidth: root.metricColumnWidth
                onClicked: function(slotIndex) {
                    root.slotClicked(slotIndex)
                }
                onPlayerDroppedOnSlot: function(playerId, slotIndex) {
                    root.playerDroppedOnSlot(playerId, slotIndex)
                }
                onSlotDroppedOnSlot: function(sourceSlotIndex, targetSlotIndex) {
                    root.slotDroppedOnSlot(sourceSlotIndex, targetSlotIndex)
                }
            }
        }
    }
}

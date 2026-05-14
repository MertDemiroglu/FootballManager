import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    property var slotsModel: null
    property var substitutesModel: null
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1
    property int metricColumnWidth: 58
    property string lineupPanelMode: "StartingXI"

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
        Layout.rightMargin: 8
        spacing: 8

        RowLayout {
            Layout.preferredWidth: 238
            Layout.preferredHeight: 32
            spacing: 6

            Repeater {
                model: [
                    { label: "Starting XI", mode: "StartingXI" },
                    { label: "Substitutes", mode: "Substitutes" }
                ]

                delegate: Button {
                    required property var modelData
                    text: modelData.label
                    Layout.preferredWidth: 116
                    Layout.preferredHeight: 32
                    onClicked: root.lineupPanelMode = modelData.mode
                    contentItem: Label {
                        text: parent.text
                        color: root.lineupPanelMode === parent.modelData.mode ? "#f7fbff" : "#d7e2ec"
                        font.pixelSize: 12
                        font.bold: root.lineupPanelMode === parent.modelData.mode
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 7
                        color: root.lineupPanelMode === parent.modelData.mode ? "#105e34" : "#111c28"
                        border.color: root.lineupPanelMode === parent.modelData.mode ? "#2fb565" : "#33485a"
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Overall"; font.pixelSize: 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Form"; font.pixelSize: 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Fitness"; font.pixelSize: 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Moral"; font.pixelSize: 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2
        visible: root.lineupPanelMode === "StartingXI"

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

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: Math.max(120, Math.min(10, substituteList.count) * 44 + 12)
        radius: 8
        visible: root.lineupPanelMode === "Substitutes"
        color: "#0b1520"
        border.color: "#1f3040"
        clip: true

        ListView {
            id: substituteList
            anchors.fill: parent
            anchors.margins: 6
            model: root.substitutesModel && root.substitutesModel.rows ? root.substitutesModel.rows : []
            spacing: 2
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: LineupRosterRow {
                width: ListView.view ? ListView.view.width - 4 : 0
                selectedPlayerId: 0
                rowData: modelData
                metricColumnWidth: root.metricColumnWidth
            }
        }
    }
}

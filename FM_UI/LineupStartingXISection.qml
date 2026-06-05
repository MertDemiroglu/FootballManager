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
    property var metrics: null
    readonly property var orderedSlotRows: orderedSlots()
    readonly property var substituteRows: substitutesModel && substitutesModel.rows ? substitutesModel.rows : []
    readonly property int rowHeight: metrics ? Math.round((metrics.dense ? 32 : 42) * metrics.visualScale) : 42
    readonly property int rowSpacing: 2
    readonly property int tabHeaderHeight: metrics ? Math.round((metrics.dense ? 30 : 32) * metrics.visualScale) : 32
    readonly property int tableInnerMargin: metrics ? metrics.spacingSm : 8
    readonly property int tableRowCapacity: Math.max(orderedSlotRows.length, substituteRows.length, 11)
    readonly property int rowsHeight: tableRowCapacity * rowHeight
                                      + Math.max(0, tableRowCapacity - 1) * rowSpacing

    implicitHeight: tabHeaderHeight + spacing + rowsHeight + tableInnerMargin * 2

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

    spacing: metrics ? metrics.spacingSm : 8

    RowLayout {
        Layout.fillWidth: true
        Layout.rightMargin: metrics ? metrics.spacingSm : 8
        Layout.preferredHeight: root.tabHeaderHeight
        spacing: metrics ? metrics.spacingSm : 8

        RowLayout {
            Layout.preferredWidth: root.metrics ? root.metrics.px(root.metrics.dense ? 204 : 238) : 238
            Layout.preferredHeight: root.tabHeaderHeight
            spacing: root.metrics ? root.metrics.spacingXs : 6

            Repeater {
                model: [
                    { label: "Starting XI", mode: "StartingXI" },
                    { label: "Substitutes", mode: "Substitutes" }
                ]

                delegate: Button {
                    id: tabButton
                    required property var modelData
                    readonly property bool active: root.lineupPanelMode === modelData.mode
                    text: modelData.label
                    Layout.preferredWidth: root.metrics ? root.metrics.px(root.metrics.dense ? 98 : 116) : 116
                    Layout.preferredHeight: root.tabHeaderHeight
                    focusPolicy: Qt.NoFocus
                    onClicked: root.lineupPanelMode = modelData.mode
                    contentItem: Label {
                        text: parent.text
                        color: tabButton.active ? "#f7fbff" : "#d7e2ec"
                        font.pixelSize: root.metrics ? root.metrics.font(12) : 12
                        font.bold: tabButton.active
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 7
                        color: tabButton.active
                               ? (tabButton.down ? "#0d4f2d" : (tabButton.hovered ? "#12683b" : "#105e34"))
                               : (tabButton.down ? "#182434" : (tabButton.hovered ? "#172536" : "#111c28"))
                        border.color: tabButton.active ? "#2fb565" : "#33485a"
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        Label { Layout.preferredWidth: root.metricColumnWidth; text: root.metrics && root.metrics.dense ? "OVR" : "Overall"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Form"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: root.metrics && root.metrics.dense ? "Fit" : "Fitness"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Moral"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: root.rowsHeight + root.tableInnerMargin * 2
        radius: root.metrics ? root.metrics.radiusMd : 8
        color: "#0b1520"
        border.color: "#1f3040"
        clip: true

        ColumnLayout {
            id: tableBody
            anchors.fill: parent
            anchors.margins: root.tableInnerMargin
            spacing: root.rowSpacing

            Repeater {
                model: root.lineupPanelMode === "StartingXI" ? root.orderedSlotRows : []

                delegate: LineupStartingXIRow {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.rowHeight
                    slotData: modelData
                    selectedSlotIndex: root.selectedSlotIndex
                    selectedSourceSlotIndex: root.selectedSourceSlotIndex
                    metricColumnWidth: root.metricColumnWidth
                    metrics: root.metrics
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

            Repeater {
                model: root.lineupPanelMode === "Substitutes" ? root.substituteRows : []

                delegate: LineupRosterRow {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.rowHeight
                    selectedPlayerId: 0
                    rowData: modelData
                    metricColumnWidth: root.metricColumnWidth
                    metrics: root.metrics
                }
            }
        }
    }
}

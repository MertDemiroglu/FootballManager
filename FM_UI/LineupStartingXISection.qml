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
        Layout.preferredHeight: metrics ? metrics.px(metrics.dense ? 30 : 32) : 32
        spacing: metrics ? metrics.spacingSm : 8

        RowLayout {
            Layout.preferredWidth: root.metrics ? root.metrics.px(root.metrics.dense ? 204 : 238) : 238
            Layout.preferredHeight: root.metrics ? root.metrics.px(root.metrics.dense ? 30 : 32) : 32
            spacing: root.metrics ? root.metrics.spacingXs : 6

            Repeater {
                model: [
                    { label: "Starting XI", mode: "StartingXI" },
                    { label: "Substitutes", mode: "Substitutes" }
                ]

                delegate: Button {
                    required property var modelData
                    text: modelData.label
                    Layout.preferredWidth: root.metrics ? root.metrics.px(root.metrics.dense ? 98 : 116) : 116
                    Layout.preferredHeight: root.metrics ? root.metrics.px(root.metrics.dense ? 30 : 32) : 32
                    onClicked: root.lineupPanelMode = modelData.mode
                    contentItem: Label {
                        text: parent.text
                        color: root.lineupPanelMode === parent.modelData.mode ? "#f7fbff" : "#d7e2ec"
                        font.pixelSize: root.metrics ? root.metrics.font(12) : 12
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

        Label { Layout.preferredWidth: root.metricColumnWidth; text: root.metrics && root.metrics.dense ? "OVR" : "Overall"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Form"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: root.metrics && root.metrics.dense ? "Fit" : "Fitness"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        Label { Layout.preferredWidth: root.metricColumnWidth; text: "Moral"; font.pixelSize: root.metrics ? root.metrics.font(12) : 12; font.bold: true; color: "#f2f7ff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
    }

    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: root.metrics ? root.metrics.px(116) : 116
        spacing: 2
        visible: root.lineupPanelMode === "StartingXI"
        clip: true
        model: root.orderedSlotRows
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        delegate: LineupStartingXIRow {
            width: ListView.view ? ListView.view.width - 8 : 0
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

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: root.metrics ? root.metrics.px(116) : 116
        radius: root.metrics ? root.metrics.radiusMd : 8
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
                metrics: root.metrics
            }
        }
    }
}

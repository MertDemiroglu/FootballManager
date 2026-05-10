import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "TeamVisuals.js" as TeamVisuals

Rectangle {
    id: root

    property var slotsModel: null
    property var slotRows: slotsModel ? slotsModel.rows : []
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1
    property string teamName: gameFacade.shellState.selectedTeamName || ""

    signal slotClicked(int slotIndex)
    signal playerDroppedOnSlot(int playerId, int slotIndex)
    signal slotDroppedOnSlot(int sourceSlotIndex, int targetSlotIndex)

    radius: 14
    border.color: "#1d5f3c"
    color: "#0b1520"
    clip: true
    Layout.minimumHeight: 420
    layer.enabled: true
    layer.smooth: true

    function normalized(value, fallback) {
        return typeof value === "number" ? Math.max(0, Math.min(1, value)) : fallback
    }

    function clamped(value, minValue, maxValue) {
        if (maxValue < minValue)
            return minValue
        return Math.max(minValue, Math.min(maxValue, value))
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 0

        FootballPitchSurface {
            id: pitchBackground
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 360

            Item {
                id: cardsLayer
                anchors.fill: pitchBackground.fieldItem
                visible: root.slotRows.length > 0

                readonly property real cardWidth: 108
                readonly property real cardHeight: 122

                Repeater {
                    model: root.slotRows

                    delegate: LineupSlotCard {
                        width: cardsLayer.cardWidth
                        height: cardsLayer.cardHeight
                        slotData: modelData
                        selectedSlotIndex: root.selectedSlotIndex
                        selectedSourceSlotIndex: root.selectedSourceSlotIndex
                        kitColorPrimary: TeamVisuals.primaryColor(root.teamName)
                        kitColorSecondary: TeamVisuals.secondaryColor(root.teamName)
                        x: root.clamped(
                               root.normalized(modelData.pitchX, 0.5) * cardsLayer.width - width / 2,
                               0,
                               cardsLayer.width - width)
                        y: root.clamped(
                               root.normalized(modelData.pitchY, 0.5) * cardsLayer.height - height / 2,
                               0,
                               cardsLayer.height - height)
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

            Label {
                anchors.centerIn: parent
                visible: root.slotRows.length === 0
                text: "No formation slots loaded"
                color: "#ffffff"
                font.pixelSize: 14
            }
        }
    }
}

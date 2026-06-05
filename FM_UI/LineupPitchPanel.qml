import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var slotsModel: null
    property var slotRows: slotsModel ? slotsModel.rows : []
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1
    property string teamName: gameFacade.shellState.selectedTeamName || ""
    property string teamPrimaryColor: gameFacade.shellState.selectedTeamPrimaryColor || "#22c55e"
    property string teamSecondaryColor: gameFacade.shellState.selectedTeamSecondaryColor || "#0f172a"
    property var metrics: null
    property bool preservePitchAspectRatio: false
    property real pitchAspectRatio: 0.68

    signal slotClicked(int slotIndex)
    signal playerDroppedOnSlot(int playerId, int slotIndex)
    signal slotDroppedOnSlot(int sourceSlotIndex, int targetSlotIndex)

    radius: metrics ? metrics.radiusLg : 14
    border.color: "#1d5f3c"
    color: "#0b1520"
    clip: true
    Layout.minimumHeight: metrics ? Math.round(420 * metrics.visualScale) : 420
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
        anchors.margins: root.metrics ? root.metrics.spacingSm : 10
        spacing: 0

        Item {
            id: pitchBounds
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: root.metrics ? Math.round(360 * root.metrics.visualScale) : 360

            FootballPitchSurface {
                id: pitchBackground
                width: root.preservePitchAspectRatio
                       ? Math.min(pitchBounds.width, pitchBounds.height * root.pitchAspectRatio)
                       : pitchBounds.width
                height: root.preservePitchAspectRatio
                        ? Math.min(pitchBounds.height, pitchBounds.width / root.pitchAspectRatio)
                        : pitchBounds.height
                anchors.centerIn: parent
                metrics: root.metrics

                Item {
                    id: cardsLayer
                    anchors.fill: pitchBackground.fieldItem
                    visible: root.slotRows.length > 0

                    readonly property real cardScale: root.metrics
                                                      ? root.metrics.clamp(Math.min(width / 540, height / 560), 0.72, 1.0)
                                                      : Math.max(0.72, Math.min(1.0, Math.min(width / 540, height / 560)))
                    readonly property real cardWidth: Math.round(108 * cardScale)
                    readonly property real cardHeight: Math.round(122 * cardScale)

                    Repeater {
                        model: root.slotRows

                        delegate: LineupSlotCard {
                            width: cardsLayer.cardWidth
                            height: cardsLayer.cardHeight
                            slotData: modelData
                            selectedSlotIndex: root.selectedSlotIndex
                            selectedSourceSlotIndex: root.selectedSourceSlotIndex
                            kitColorPrimary: root.teamPrimaryColor
                            kitColorSecondary: root.teamSecondaryColor
                            metrics: root.metrics
                            scaleFactor: cardsLayer.cardScale
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
                    font.pixelSize: root.metrics ? root.metrics.font(14) : 14
                }
            }
        }
    }
}

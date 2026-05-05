import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var slotsModel: null
    property var slotRows: slotsModel ? slotsModel.rows : []
    property int selectedSlotIndex: -1
    property int selectedSourceSlotIndex: -1

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

        Rectangle {
            id: pitchBackground
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 360
            radius: 12
            color: "#0b5c34"
            border.color: "#237447"
            border.width: 1
            clip: true

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#0d5f37" }
                GradientStop { position: 0.46; color: "#073f28" }
                GradientStop { position: 1.0; color: "#052c20" }
            }

            Repeater {
                model: 8
                delegate: Rectangle {
                    required property int index
                    x: 0
                    y: pitchBackground.height * index / 8
                    width: pitchBackground.width
                    height: pitchBackground.height / 8
                    color: index % 2 === 0 ? "#ffffff" : "#000000"
                    opacity: index % 2 === 0 ? 0.025 : 0.05
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"
                border.width: 12
                border.color: "#031910"
                opacity: 0.22
            }

            Rectangle {
                id: pitchField
                anchors.fill: parent
                anchors.margins: 16
                radius: 10
                color: "transparent"
                border.width: 2
                border.color: "#a9d1b5"
                opacity: 0.62
            }

            Rectangle {
                anchors.left: pitchField.left
                anchors.right: pitchField.right
                anchors.verticalCenter: pitchField.verticalCenter
                height: 2
                color: "#a9d1b5"
                opacity: 0.58
            }

            Rectangle {
                anchors.centerIn: pitchField
                width: Math.min(pitchField.width, pitchField.height) * 0.22
                height: width
                radius: width / 2
                color: "transparent"
                border.width: 2
                border.color: "#a9d1b5"
                opacity: 0.56
            }

            Rectangle {
                anchors.horizontalCenter: pitchField.horizontalCenter
                anchors.top: pitchField.top
                width: pitchField.width * 0.42
                height: pitchField.height * 0.16
                color: "transparent"
                border.width: 2
                border.color: "#a9d1b5"
                opacity: 0.54
            }

            Rectangle {
                anchors.horizontalCenter: pitchField.horizontalCenter
                anchors.bottom: pitchField.bottom
                width: pitchField.width * 0.42
                height: pitchField.height * 0.16
                color: "transparent"
                border.width: 2
                border.color: "#a9d1b5"
                opacity: 0.54
            }

            Rectangle {
                anchors.horizontalCenter: pitchField.horizontalCenter
                anchors.top: pitchField.top
                width: pitchField.width * 0.22
                height: pitchField.height * 0.07
                color: "transparent"
                border.width: 2
                border.color: "#a9d1b5"
                opacity: 0.5
            }

            Rectangle {
                anchors.horizontalCenter: pitchField.horizontalCenter
                anchors.bottom: pitchField.bottom
                width: pitchField.width * 0.22
                height: pitchField.height * 0.07
                color: "transparent"
                border.width: 2
                border.color: "#a9d1b5"
                opacity: 0.5
            }

            Item {
                id: cardsLayer
                anchors.fill: pitchField
                visible: root.slotRows.length > 0

                readonly property real cardWidth: Math.max(124, Math.min(158, width * 0.19))
                readonly property real cardHeight: Math.max(64, Math.min(78, height * 0.14))

                Repeater {
                    model: root.slotRows

                    delegate: LineupSlotCard {
                        width: cardsLayer.cardWidth
                        height: cardsLayer.cardHeight
                        slotData: modelData
                        selectedSlotIndex: root.selectedSlotIndex
                        selectedSourceSlotIndex: root.selectedSourceSlotIndex
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

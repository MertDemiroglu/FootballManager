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

    radius: 14
    border.color: "#d8dee8"
    color: "#ffffff"
    clip: true
    Layout.minimumHeight: 420

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
        anchors.margins: 16
        spacing: 10

        Label {
            text: "Pitch"
            font.pixelSize: 18
            font.bold: true
            color: "#17212f"
        }

        Rectangle {
            id: pitchBackground
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 360
            radius: 12
            color: "#187343"
            border.color: "#0d5130"
            clip: true

            Rectangle {
                id: pitchField
                anchors.fill: parent
                anchors.margins: 18
                radius: 10
                color: "transparent"
                border.width: 2
                border.color: "#e8fff1"
            }

            Rectangle {
                anchors.left: pitchField.left
                anchors.right: pitchField.right
                anchors.verticalCenter: pitchField.verticalCenter
                height: 2
                color: "#d9fbe7"
                opacity: 0.85
            }

            Rectangle {
                anchors.centerIn: pitchField
                width: Math.min(pitchField.width, pitchField.height) * 0.22
                height: width
                radius: width / 2
                color: "transparent"
                border.width: 2
                border.color: "#d9fbe7"
                opacity: 0.85
            }

            Rectangle {
                anchors.horizontalCenter: pitchField.horizontalCenter
                anchors.top: pitchField.top
                width: pitchField.width * 0.42
                height: pitchField.height * 0.16
                color: "transparent"
                border.width: 2
                border.color: "#d9fbe7"
                opacity: 0.8
            }

            Rectangle {
                anchors.horizontalCenter: pitchField.horizontalCenter
                anchors.bottom: pitchField.bottom
                width: pitchField.width * 0.42
                height: pitchField.height * 0.16
                color: "transparent"
                border.width: 2
                border.color: "#d9fbe7"
                opacity: 0.8
            }

            Item {
                id: cardsLayer
                anchors.fill: pitchField
                visible: root.slotRows.length > 0

                readonly property real cardWidth: Math.max(118, Math.min(146, width * 0.20))
                readonly property real cardHeight: Math.max(56, Math.min(66, height * 0.13))

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

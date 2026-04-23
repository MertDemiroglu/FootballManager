import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var slotData: ({})

    radius: 8
    color: slotData.isEmpty ? "#fef3c7" : "#f8fafc"
    border.color: slotData.isEmpty ? "#f59e0b" : "#d0d5dd"
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
}

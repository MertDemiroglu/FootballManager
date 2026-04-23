import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var slotRows: []

    radius: 14
    border.color: "#d8dee8"
    color: "#ffffff"
    implicitHeight: 520

    function roleGroup(slotRow) {
        const role = slotRow.slotRole || ""
        if (role === "GK")
            return 0
        if (role === "LB" || role === "CB" || role === "RB" || role === "LWB" || role === "RWB")
            return 1
        if (role === "DM" || role === "CM" || role === "AM" || role === "LM" || role === "RM")
            return 2
        return 3
    }

    function slotsForGroup(groupIndex) {
        const grouped = []
        for (let i = 0; i < slotRows.length; ++i) {
            const row = slotRows[i]
            if (roleGroup(row) === groupIndex)
                grouped.push(row)
        }
        return grouped
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
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "#1e7a46"
            border.color: "#0f5132"

            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.88
                height: parent.height * 0.88
                color: "transparent"
                border.width: 2
                border.color: "#ffffff"
                radius: 10
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width * 0.88
                height: 2
                color: "#ffffff"
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 22
                spacing: 10

                Repeater {
                    model: 4

                    delegate: RowLayout {
                        required property int index
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 8

                        Repeater {
                            model: root.slotsForGroup(index)
                            delegate: LineupSlotCard {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                slotData: modelData
                            }
                        }
                    }
                }
            }
        }
    }
}

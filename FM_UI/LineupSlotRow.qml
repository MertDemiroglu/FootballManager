import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property var rowData: ({})

    width: parent ? parent.width : 0
    radius: 8
    color: "#f8fafc"
    border.color: "#e4e7ec"
    implicitHeight: 42

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Rectangle {
            Layout.preferredWidth: 36
            Layout.preferredHeight: 24
            radius: 6
            color: "#eef2ff"
            border.color: "#d0d5dd"

            Label {
                anchors.centerIn: parent
                text: rowData.slotRoleText || "?"
                color: "#344054"
                font.bold: true
                font.pixelSize: 11
            }
        }

        Label {
            Layout.fillWidth: true
            text: rowData.playerName || "Unknown"
            color: "#101828"
            font.pixelSize: 13
            font.bold: true
            elide: Text.ElideRight
        }

        Label {
            text: rowData.positionText || "-"
            color: "#667085"
            font.pixelSize: 12
            visible: !compactMode
        }

        Label {
            text: "OVR " + (rowData.overall !== undefined ? rowData.overall : "-")
            color: "#475467"
            font.pixelSize: 12
        }
    }

    property bool compactMode: false
}

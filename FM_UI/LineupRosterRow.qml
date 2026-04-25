import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var rowData: ({})
    property int selectedPlayerId: 0
    readonly property int playerId: rowData.playerId || 0
    readonly property bool isSelected: playerId > 0 && playerId === selectedPlayerId

    signal clicked(int playerId)

    radius: 8
    border.color: isSelected ? "#2563eb" : (rowData.isAssigned ? "#93c5fd" : "#e4e7ec")
    border.width: isSelected ? 2 : 1
    color: isSelected ? "#dbeafe" : (rowData.isAssigned ? "#eff6ff" : "#ffffff")
    implicitHeight: 50

    RowLayout {
        id: rosterContent
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 8

        PositionBadge {
            text: rowData.positionShort || "?"
            implicitHeight: 24
        }

        Label {
            text: rowData.overallSummary || ("OVR " + (rowData.overall || "-"))
            font.pixelSize: 11
            font.bold: true
            color: "#475467"
        }

        Label {
            Layout.fillWidth: true
            text: rowData.name || "Unknown"
            font.pixelSize: 13
            font.bold: true
            color: "#101828"
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            visible: rowData.isAssigned
            radius: 999
            color: "#dbeafe"
            border.color: "#60a5fa"
            implicitWidth: 34
            implicitHeight: 22

            Label {
                anchors.centerIn: parent
                text: "S#" + rowData.assignedSlotIndex
                font.pixelSize: 10
                font.bold: true
                color: "#1d4ed8"
            }
        }

        ConditionBadge {
            label: "F"
            value: rowData.form
            compact: true
        }

        ConditionBadge {
            label: "Fit"
            value: rowData.fitness
            compact: true
        }

        ConditionBadge {
            label: "M"
            value: rowData.morale
            compact: true
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (root.playerId > 0) {
                root.clicked(root.playerId)
            }
        }
    }
}

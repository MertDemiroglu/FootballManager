import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property string title: ""
    property string teamName: ""
    property string coachName: ""
    property string formationText: ""
    property var lineupRows: []
    property bool compactMode: false

    radius: 12
    color: "white"
    border.color: "#e4e7ec"
    implicitHeight: content.implicitHeight + 24

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            text: root.title
            color: "#475467"
            font.pixelSize: 12
            visible: text.length > 0
        }

        Label {
            text: root.teamName.length > 0 ? root.teamName : "Team"
            color: "#101828"
            font.pixelSize: 17
            font.bold: true
        }

        Label {
            text: "Coach: " + (root.coachName.length > 0 ? root.coachName : "-")
            color: "#475467"
            font.pixelSize: 12
        }

        Label {
            text: "Formation: " + (root.formationText.length > 0 ? root.formationText : "-")
            color: "#475467"
            font.pixelSize: 12
        }

        Rectangle {
            Layout.fillWidth: true
            color: "transparent"
            visible: (root.lineupRows || []).length > 0
            implicitHeight: list.implicitHeight

            Column {
                id: list
                width: parent.width
                spacing: 6

                Repeater {
                    model: root.lineupRows || []
                    delegate: LineupSlotRow {
                        width: list.width
                        rowData: modelData
                        compactMode: root.compactMode
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: (root.lineupRows || []).length === 0
            text: "Lineup unavailable"
            color: "#667085"
            font.pixelSize: 13
        }
    }
}

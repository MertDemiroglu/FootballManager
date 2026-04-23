import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var rosterRows: []

    radius: 14
    border.color: "#d8dee8"
    color: "#ffffff"
    implicitHeight: 520

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Label {
            text: "Roster"
            font.pixelSize: 18
            font.bold: true
            color: "#17212f"
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                width: parent.width
                model: root.rosterRows
                spacing: 8

                delegate: LineupRosterRow {
                    width: ListView.view ? ListView.view.width : 0
                    rowData: modelData
                }
            }
        }
    }
}

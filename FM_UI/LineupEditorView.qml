import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // GameFacade/backend remains the single source of truth for lineup editor data.
    property var gameFacade
    // Read-only is the current mode only; interactive phases will extend this view.
    property bool readOnly: true

    property var lineupSummary: ({})
    property var slotRows: []
    property var rosterRows: []

    implicitHeight: layoutRoot.implicitHeight

    function refreshData() {
        if (!gameFacade) {
            lineupSummary = ({})
            slotRows = []
            rosterRows = []
            return
        }

        lineupSummary = gameFacade.getEditableLineupSummary()
        slotRows = gameFacade.getEditableLineupSlots()
        rosterRows = gameFacade.getEditableLineupRoster()
    }

    Component.onCompleted: refreshData()

    Connections {
        target: gameFacade
        function onGameStateChanged() {
            root.refreshData()
        }
    }

    ColumnLayout {
        id: layoutRoot
        width: parent ? parent.width : 0
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: root.readOnly ? "Lineup Editor (Read-Only)" : "Lineup Editor"
            font.pixelSize: 24
            font.bold: true
            color: "#17212f"
        }

        Label {
            Layout.fillWidth: true
            text: lineupSummary.hasLineup
                  ? "Formation " + (lineupSummary.formationText || "-") + " • Assigned " + (lineupSummary.assignedCount || 0) + "/" + (lineupSummary.slotCount || 0)
                  : "Lineup data unavailable"
            font.pixelSize: 14
            color: "#526071"
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            LineupPitchPanel {
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                slotRows: root.slotRows
            }

            LineupRosterPanel {
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                rosterRows: root.rosterRows
            }
        }
    }
}

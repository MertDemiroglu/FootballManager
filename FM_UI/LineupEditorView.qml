import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // GameFacade/backend remains the single source of truth for lineup editor data.
    property var gameFacade
    // Read-only is the current mode only; interactive phases will extend this view.
    property bool readOnly: true
    property var lineupState: gameFacade ? gameFacade.editableLineupState : null
    property var slotsModel: gameFacade ? gameFacade.editableLineupSlotsModel : null
    property var rosterModel: gameFacade ? gameFacade.editableLineupRosterModel : null

    implicitHeight: layoutRoot.implicitHeight

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
            text: lineupState && lineupState.hasLineup
                  ? "Formation " + (lineupState.formationText || "-") + " • Assigned " + (lineupState.assignedCount || 0) + "/" + (lineupState.slotCount || 0)
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
                slotsModel: root.slotsModel
            }

            LineupRosterPanel {
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                rosterModel: root.rosterModel
            }
        }
    }
}

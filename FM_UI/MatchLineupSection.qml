import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var homeLineupRows: []
    property var awayLineupRows: []
    property string homeTeamName: ""
    property string awayTeamName: ""
    property string homeCoachName: ""
    property string awayCoachName: ""
    property string homeFormationText: ""
    property string awayFormationText: ""
    property bool compactMode: false
    property var metrics: null

    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: root.metrics ? root.metrics.spacingSm : 10

        Label {
            text: "Lineups"
            color: "#101828"
            font.pixelSize: root.metrics ? root.metrics.font(16) : 16
            font.bold: true
        }

        Loader {
            Layout.fillWidth: true
            sourceComponent: width >= (root.metrics ? root.metrics.px(760) : 760) ? rowLayout : columnLayout
        }
    }

    Component {
        id: rowLayout
        RowLayout {
            width: parent ? parent.width : 0
            spacing: root.metrics ? root.metrics.spacingSm : 10

            TeamLineupCard {
                Layout.fillWidth: true
                title: "Home"
                teamName: root.homeTeamName
                coachName: root.homeCoachName
                formationText: root.homeFormationText
                lineupRows: root.homeLineupRows
                compactMode: root.compactMode
                metrics: root.metrics
            }

            TeamLineupCard {
                Layout.fillWidth: true
                title: "Away"
                teamName: root.awayTeamName
                coachName: root.awayCoachName
                formationText: root.awayFormationText
                lineupRows: root.awayLineupRows
                compactMode: root.compactMode
                metrics: root.metrics
            }
        }
    }

    Component {
        id: columnLayout
        ColumnLayout {
            width: parent ? parent.width : 0
            spacing: root.metrics ? root.metrics.spacingSm : 10

            TeamLineupCard {
                Layout.fillWidth: true
                title: "Home"
                teamName: root.homeTeamName
                coachName: root.homeCoachName
                formationText: root.homeFormationText
                lineupRows: root.homeLineupRows
                compactMode: root.compactMode
                metrics: root.metrics
            }

            TeamLineupCard {
                Layout.fillWidth: true
                title: "Away"
                teamName: root.awayTeamName
                coachName: root.awayCoachName
                formationText: root.awayFormationText
                lineupRows: root.awayLineupRows
                compactMode: root.compactMode
                metrics: root.metrics
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import "TeamVisuals.js" as TeamVisuals

Item {
    id: root

    property string teamName: ""
    property int badgeSize: 64
    readonly property var palette: TeamVisuals.palette(teamName)

    implicitWidth: badgeSize
    implicitHeight: badgeSize

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "#f8fafc"
        border.color: root.palette.secondary
        border.width: 2

        Rectangle {
            width: parent.width * 0.5
            height: parent.height * 0.62
            anchors.centerIn: parent
            radius: Math.max(6, parent.width * 0.12)
            color: root.palette.primary
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            width: parent.width * 0.14
            height: parent.height * 0.62
            anchors.centerIn: parent
            radius: Math.max(3, parent.width * 0.06)
            color: root.palette.secondary
        }

        Label {
            anchors.centerIn: parent
            text: TeamVisuals.initial(root.teamName)
            color: root.palette.text
            font.pixelSize: Math.max(16, root.badgeSize * 0.32)
            font.bold: true
        }
    }
}

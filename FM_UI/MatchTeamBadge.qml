import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string teamName: ""
    property string primaryColor: "#22c55e"
    property string secondaryColor: "#0f172a"
    property string textColor: "#f8fafc"
    property int badgeSize: 64
    property var metrics: null
    property real scaleFactor: metrics ? metrics.visualScale : 1.0
    readonly property int scaledBadgeSize: Math.max(24, Math.round(badgeSize * scaleFactor))

    function initial(value) {
        const text = String(value || "").trim()
        return text.length > 0 ? text.charAt(0).toUpperCase() : "-"
    }

    implicitWidth: scaledBadgeSize
    implicitHeight: scaledBadgeSize

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "#f8fafc"
        border.color: root.secondaryColor || "#0f172a"
        border.width: 2

        Rectangle {
            width: parent.width * 0.5
            height: parent.height * 0.62
            anchors.centerIn: parent
            radius: Math.max(6, parent.width * 0.12)
            color: root.primaryColor || "#22c55e"
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            width: parent.width * 0.14
            height: parent.height * 0.62
            anchors.centerIn: parent
            radius: Math.max(3, parent.width * 0.06)
            color: root.secondaryColor || "#0f172a"
        }

        Label {
            anchors.centerIn: parent
            text: root.initial(root.teamName)
            color: root.textColor || "#f8fafc"
            font.pixelSize: Math.max(12, Math.round(root.scaledBadgeSize * 0.32))
            font.bold: true
        }
    }
}

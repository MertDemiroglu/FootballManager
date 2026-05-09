import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string teamName: ""
    property int badgeSize: 64
    readonly property var palette: teamPalette(teamName)

    implicitWidth: badgeSize
    implicitHeight: badgeSize

    function normalizedTeamName(value) {
        return String(value || "")
            .normalize("NFD")
            .replace(/[\u0300-\u036f]/g, "")
            .toLowerCase()
    }

    function teamPalette(value) {
        const name = normalizedTeamName(value)
        if (name.indexOf("alanyaspor") >= 0) {
            return { base: "#f97316", accent: "#22c55e", text: "#071016" }
        }
        if (name.indexOf("basaksehir") >= 0) {
            return { base: "#1d4ed8", accent: "#f97316", text: "#f8fafc" }
        }
        if (name.indexOf("rizespor") >= 0) {
            return { base: "#16a34a", accent: "#f8fafc", text: "#071016" }
        }
        if (name.indexOf("trabzonspor") >= 0) {
            return { base: "#7f1d1d", accent: "#2563eb", text: "#f8fafc" }
        }
        if (name.indexOf("samsunspor") >= 0) {
            return { base: "#dc2626", accent: "#f8fafc", text: "#071016" }
        }
        return { base: "#0f1a24", accent: "#22c55e", text: "#f8fafc" }
    }

    function teamInitial(value) {
        const text = String(value || "").trim()
        return text.length > 0 ? text.charAt(0).toUpperCase() : "-"
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "#f8fafc"
        border.color: root.palette.accent
        border.width: 2

        Rectangle {
            width: parent.width * 0.5
            height: parent.height * 0.62
            anchors.centerIn: parent
            radius: Math.max(6, parent.width * 0.12)
            color: root.palette.base
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            width: parent.width * 0.14
            height: parent.height * 0.62
            anchors.centerIn: parent
            radius: Math.max(3, parent.width * 0.06)
            color: root.palette.accent
        }

        Label {
            anchors.centerIn: parent
            text: root.teamInitial(root.teamName)
            color: root.palette.text
            font.pixelSize: Math.max(16, root.badgeSize * 0.32)
            font.bold: true
        }
    }
}

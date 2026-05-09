import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property var lineupRows: []
    property string formationText: ""
    property string mode: "preMatch"
    property string kitColorPrimary: "#f97316"
    property string kitColorSecondary: "#22c55e"
    property bool showMetrics: mode === "postMatch"

    radius: 10
    color: "#15351f"
    border.color: "#526c55"
    border.width: 2
    clip: true

    function positionGroup(roleText) {
        const role = String(roleText || "").toUpperCase()
        if (role === "GK") {
            return "GK"
        }
        if (role.indexOf("B") >= 0) {
            return "DEF"
        }
        if (role.indexOf("M") >= 0) {
            return "MID"
        }
        if (role.indexOf("ST") >= 0 || role.indexOf("F") >= 0 || role.indexOf("W") >= 0) {
            return "FWD"
        }
        return "UNK"
    }

    function roleOccurrence(rows, index, roleText) {
        let count = 0
        for (let i = 0; i <= index && i < rows.length; ++i) {
            if (String(rows[i].slotRoleText || "").toUpperCase() === String(roleText || "").toUpperCase()) {
                count += 1
            }
        }
        return count
    }

    function tokenPoint(rows, index, roleText) {
        const role = String(roleText || "").toUpperCase()
        const occurrence = roleOccurrence(rows, index, role)
        if (role === "GK") {
            return { x: 0.5, y: 0.86 }
        }
        if (role === "LB") {
            return { x: 0.14, y: 0.67 }
        }
        if (role === "RB") {
            return { x: 0.86, y: 0.67 }
        }
        if (role === "CB") {
            return { x: occurrence === 1 ? 0.38 : 0.62, y: 0.67 }
        }
        if (role === "LM") {
            return { x: 0.14, y: 0.43 }
        }
        if (role === "RM") {
            return { x: 0.86, y: 0.43 }
        }
        if (role === "CM" || role === "DM" || role === "AM") {
            return { x: occurrence === 1 ? 0.38 : 0.62, y: 0.43 }
        }
        if (role === "ST" || role === "CF") {
            return { x: occurrence === 1 ? 0.38 : 0.62, y: 0.19 }
        }
        return { x: 0.18 + ((index % 4) * 0.21), y: 0.28 + (Math.floor(index / 4) * 0.18) }
    }

    function metricText(row) {
        if (row.matchRating !== undefined && row.matchRating > 0) {
            return Number(row.matchRating).toFixed(1)
        }
        if (row.rating !== undefined && row.rating > 0) {
            return Number(row.rating).toFixed(1)
        }
        if (root.mode === "lineupEditor" && row.overall !== undefined && row.overall > 0) {
            return String(row.overall)
        }
        return ""
    }

    Rectangle {
        anchors.fill: parent
        opacity: 0.18
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#2f5f36" }
            GradientStop { position: 1.0; color: "#0b2415" }
        }
    }

    Grid {
        anchors.fill: parent
        columns: 6
        rows: 10
        opacity: 0.08
        Repeater {
            model: 60
            Rectangle {
                width: root.width / 6
                height: root.height / 10
                color: index % 2 === 0 ? "#ffffff" : "#000000"
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        color: "transparent"
        border.color: "#d7e7d7"
        border.width: 1
        opacity: 0.32
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width * 0.18
        height: width
        radius: width / 2
        color: "transparent"
        border.color: "#d7e7d7"
        opacity: 0.28
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: 1
        color: "#d7e7d7"
        opacity: 0.24
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 10
        width: parent.width * 0.58
        height: parent.height * 0.22
        color: "transparent"
        border.color: "#d7e7d7"
        opacity: 0.28
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        width: parent.width * 0.58
        height: parent.height * 0.22
        color: "transparent"
        border.color: "#d7e7d7"
        opacity: 0.28
    }

    Repeater {
        model: root.lineupRows || []

        delegate: LineupPlayerToken {
            id: token
            readonly property var point: root.tokenPoint(root.lineupRows || [], index, modelData.slotRoleText)

            x: Math.max(4, Math.min(root.width - width - 4, root.width * point.x - width / 2))
            y: Math.max(4, Math.min(root.height - height - 4, root.height * point.y - height / 2))
            positionText: modelData.slotRoleText || modelData.positionText || "-"
            playerName: modelData.playerName || ""
            kitColorPrimary: root.kitColorPrimary
            kitColorSecondary: root.kitColorSecondary
            positionGroup: root.positionGroup(modelData.slotRoleText || modelData.positionText)
            mode: root.mode
            showMetric: root.showMetrics
            metricText: root.metricText(modelData)
            empty: modelData.hasPlayer === false
        }
    }

    Label {
        anchors.centerIn: parent
        visible: (root.lineupRows || []).length === 0
        text: "Lineup unavailable"
        color: "#a8b3c1"
        font.pixelSize: 16
    }
}

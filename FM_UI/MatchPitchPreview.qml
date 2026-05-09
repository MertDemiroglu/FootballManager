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

    function normalized(value, fallback) {
        return typeof value === "number" ? Math.max(0, Math.min(1, value)) : fallback
    }

    function fallbackPoint(index) {
        return {
            x: 0.18 + ((index % 4) * 0.21),
            y: 0.24 + (Math.floor(index / 4) * 0.18)
        }
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
        id: pitchField
        anchors.fill: parent
        anchors.margins: 18
        color: "transparent"
        border.color: "#d7e7d7"
        border.width: 2
        opacity: 0.32
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: Math.min(pitchField.width, pitchField.height) * 0.22
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
        anchors.top: pitchField.top
        width: pitchField.width * 0.58
        height: pitchField.height * 0.22
        color: "transparent"
        border.color: "#d7e7d7"
        opacity: 0.28
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: pitchField.bottom
        width: pitchField.width * 0.58
        height: pitchField.height * 0.22
        color: "transparent"
        border.color: "#d7e7d7"
        opacity: 0.28
    }

    Item {
        id: tokenLayer
        anchors.fill: pitchField
        anchors.leftMargin: 30
        anchors.rightMargin: 30
        anchors.topMargin: 34
        anchors.bottomMargin: 34

        Repeater {
            model: root.lineupRows || []

            delegate: LineupPlayerToken {
                id: token
                readonly property var fallback: root.fallbackPoint(index)
                readonly property real pointX: root.normalized(modelData.pitchX, fallback.x)
                readonly property real pointY: root.normalized(modelData.pitchY, fallback.y)

                x: Math.max(0, Math.min(tokenLayer.width - width, tokenLayer.width * pointX - width / 2))
                y: Math.max(0, Math.min(tokenLayer.height - height, tokenLayer.height * pointY - height / 2))
                positionText: modelData.slotRoleText || modelData.positionText || "-"
                playerName: modelData.playerName || ""
                surname: modelData.surname || ""
                kitColorPrimary: root.kitColorPrimary
                kitColorSecondary: root.kitColorSecondary
                positionGroup: root.positionGroup(modelData.slotRoleText || modelData.positionText)
                mode: root.mode
                showMetric: root.showMetrics && root.metricText(modelData).length > 0
                metricText: root.metricText(modelData)
                empty: modelData.hasPlayer === false
            }
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

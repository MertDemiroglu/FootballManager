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
    property var metrics: null
    property real scaleFactor: metrics ? metrics.scale : 1.0
    property bool preservePitchAspectRatio: false
    property real pitchAspectRatio: 0.68

    color: "transparent"
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
        // TODO: Populate real player match ratings from future match engine/player rating system.
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

    Item {
        id: pitchBounds
        width: root.preservePitchAspectRatio ? Math.min(root.width, root.height * root.pitchAspectRatio) : root.width
        height: root.preservePitchAspectRatio ? Math.min(root.height, root.width / root.pitchAspectRatio) : root.height
        anchors.centerIn: parent

        FootballPitchSurface {
            id: pitchSurface
            anchors.fill: parent
            fieldMargin: 28
            metrics: root.metrics
        }
    }

    Item {
        id: tokenLayer
        readonly property real insetX: pitchSurface.fieldItem.width * 0.09
        readonly property real insetY: pitchSurface.fieldItem.height * 0.08
        readonly property real tokenScale: root.metrics
                                           ? root.metrics.clamp(Math.min(width / 500, height / 620), 0.72, 1.04)
                                           : Math.max(0.72, Math.min(1.04, Math.min(width / 500, height / 620)))
        x: pitchBounds.x + pitchSurface.fieldItem.x + insetX
        y: pitchBounds.y + pitchSurface.fieldItem.y + insetY
        width: Math.max(1, pitchSurface.fieldItem.width - insetX * 2)
        height: Math.max(1, pitchSurface.fieldItem.height - insetY * 2)

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
                metrics: root.metrics
                scaleFactor: tokenLayer.tokenScale
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
        font.pixelSize: root.metrics ? root.metrics.font(16) : 16
    }
}

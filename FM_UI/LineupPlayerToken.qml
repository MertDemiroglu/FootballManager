import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string positionText: ""
    property string playerName: ""
    property string surname: ""
    property string displayName: ""
    property string kitColorPrimary: "#f97316"
    property string kitColorSecondary: "#22c55e"
    property string positionGroup: ""
    property string metricText: ""
    property bool showMetric: false
    property string mode: "preMatch"
    property bool empty: false
    property var metrics: null
    property real scaleFactor: metrics ? metrics.compactTokenScale : 1.0

    readonly property string resolvedName: root.empty
                                          ? "Empty"
                                          : (root.surname.length > 0
                                             ? root.surname
                                             : surnameFromName(root.displayName.length > 0 ? root.displayName : root.playerName))
    readonly property string resolvedGroup: root.positionGroup.length > 0
                                            ? root.positionGroup
                                            : groupFromPosition(root.positionText)

    width: Math.round(90 * scaleFactor)
    height: Math.round((root.showMetric ? 100 : 76) * scaleFactor)

    function surnameFromName(fullName) {
        const parts = String(fullName || "").trim().split(/\s+/).filter(function(part) { return part.length > 0 })
        return parts.length > 0 ? parts[parts.length - 1] : "Empty"
    }

    function groupFromPosition(position) {
        const text = String(position || "").toUpperCase()
        if (text === "GK") {
            return "GK"
        }
        if (text.indexOf("B") >= 0 || text === "DF") {
            return "DEF"
        }
        if (text.indexOf("M") >= 0) {
            return "MID"
        }
        if (text.indexOf("ST") >= 0 || text.indexOf("F") >= 0 || text.indexOf("W") >= 0) {
            return "FWD"
        }
        return "UNK"
    }

    function bandColor(group) {
        switch (group) {
        case "GK":
            return "#15803d"
        case "DEF":
            return "#1d4ed8"
        case "MID":
            return "#d97706"
        case "FWD":
            return "#b91c1c"
        default:
            return "#475569"
        }
    }

    Item {
        id: shirt
        width: Math.round(46 * root.scaleFactor)
        height: Math.round(36 * root.scaleFactor)
        anchors.horizontalCenter: parent.horizontalCenter
        y: 0
        visible: !root.empty

        Rectangle {
            x: Math.round(10 * root.scaleFactor)
            y: Math.round(7 * root.scaleFactor)
            width: Math.round(26 * root.scaleFactor)
            height: Math.round(25 * root.scaleFactor)
            radius: Math.round(7 * root.scaleFactor)
            color: root.kitColorPrimary
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            x: 0
            y: Math.round(10 * root.scaleFactor)
            width: Math.round(16 * root.scaleFactor)
            height: Math.round(14 * root.scaleFactor)
            radius: Math.round(4 * root.scaleFactor)
            rotation: -18
            color: root.kitColorSecondary
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            x: Math.round(30 * root.scaleFactor)
            y: Math.round(10 * root.scaleFactor)
            width: Math.round(16 * root.scaleFactor)
            height: Math.round(14 * root.scaleFactor)
            radius: Math.round(4 * root.scaleFactor)
            rotation: 18
            color: root.kitColorSecondary
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            x: Math.round(21 * root.scaleFactor)
            y: Math.round(7 * root.scaleFactor)
            width: Math.max(2, Math.round(5 * root.scaleFactor))
            height: Math.round(25 * root.scaleFactor)
            radius: Math.round(2 * root.scaleFactor)
            color: root.kitColorSecondary
            opacity: 0.9
        }

        Rectangle {
            x: Math.round(17 * root.scaleFactor)
            y: Math.round(6 * root.scaleFactor)
            width: Math.round(12 * root.scaleFactor)
            height: Math.round(7 * root.scaleFactor)
            radius: Math.round(4 * root.scaleFactor)
            color: "#dbeafe"
            opacity: 0.9
            border.color: "#0f172a"
            border.width: 1
        }
    }

    Rectangle {
        id: body
        anchors.horizontalCenter: parent.horizontalCenter
        y: Math.round((root.empty ? 21 : 30) * root.scaleFactor)
        width: Math.round(86 * root.scaleFactor)
        height: Math.round((root.showMetric ? 66 : 44) * root.scaleFactor)
        radius: Math.round(7 * root.scaleFactor)
        color: root.empty ? "#111827" : "#111923"
        border.color: root.empty ? "#334155" : "#405264"
        border.width: 1

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: Math.round(20 * root.scaleFactor)
            radius: Math.round(6 * root.scaleFactor)
            color: root.empty ? "#334155" : root.bandColor(root.resolvedGroup)

            Label {
                anchors.centerIn: parent
                text: root.positionText || "-"
                color: "#f8fafc"
                font.pixelSize: root.metrics ? root.metrics.font(12) : Math.round(12 * root.scaleFactor)
                font.bold: true
            }
        }

        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: Math.round(23 * root.scaleFactor)
            height: Math.round(19 * root.scaleFactor)
            text: root.resolvedName
            color: root.empty ? "#94a3b8" : "#f8fafc"
            font.pixelSize: root.metrics ? root.metrics.font(14) : Math.round(14 * root.scaleFactor)
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }

        Rectangle {
            width: Math.round(42 * root.scaleFactor)
            height: Math.round(18 * root.scaleFactor)
            radius: Math.round(8 * root.scaleFactor)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Math.round(5 * root.scaleFactor)
            visible: root.showMetric
            color: "#0b1118"
            border.color: "#405264"

            Label {
                anchors.centerIn: parent
                text: root.metricText.length > 0 ? root.metricText : "-"
                color: root.metricText.length > 0 ? "#f8fafc" : "#64748b"
                font.pixelSize: root.metrics ? root.metrics.font(11) : Math.round(11 * root.scaleFactor)
                font.bold: true
            }
        }
    }
}

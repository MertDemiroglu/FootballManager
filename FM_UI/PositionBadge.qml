import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string text: "?"
    property int roleKey: -1
    property int horizontalPadding: 9
    property var metrics: null
    property real scaleFactor: metrics ? metrics.scale : 1.0

    function normalizedLabel() {
        return (root.text || "?").toUpperCase()
    }

    function groupForLabel(label) {
        if (label === "GK")
            return "gk"
        if (label === "CB" || label === "LB" || label === "RB")
            return "defense"
        if (label === "LWB" || label === "RWB" || label === "LM" || label === "RM" || label === "LW" || label === "RW")
            return "wide"
        if (label === "DM" || label === "CM" || label === "AM")
            return "midfield"
        if (label === "ST" || label === "CF" || label === "FW")
            return "forward"
        return "unknown"
    }

    function palette(group, shade) {
        const colors = {
            gk: { fill: "#123821", border: "#2fd06f", text: "#9df2ba" },
            defense: { fill: "#122f55", border: "#4a90ff", text: "#b8d7ff" },
            midfield: { fill: "#3a2f13", border: "#f4c542", text: "#ffe39b" },
            wide: { fill: "#3a2214", border: "#f08a35", text: "#ffc18a" },
            forward: { fill: "#3b171c", border: "#ef5d68", text: "#ffb1b8" },
            unknown: { fill: "#263241", border: "#6c7c8c", text: "#d3dde6" }
        }
        return colors[group][shade]
    }

    readonly property string badgeText: normalizedLabel()
    readonly property string badgeGroup: groupForLabel(badgeText)

    radius: Math.round(6 * scaleFactor)
    color: palette(badgeGroup, "fill")
    border.color: palette(badgeGroup, "border")
    implicitWidth: badgeLabel.implicitWidth + Math.round(horizontalPadding * scaleFactor) * 2
    implicitHeight: Math.round(24 * scaleFactor)

    Label {
        id: badgeLabel
        anchors.centerIn: parent
        text: root.badgeText
        font.bold: true
        font.pixelSize: metrics ? metrics.font(11) : Math.round(11 * scaleFactor)
        color: root.palette(root.badgeGroup, "text")
    }
}

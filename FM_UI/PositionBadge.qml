import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string text: "?"
    property int roleKey: -1
    property int horizontalPadding: 9

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
            gk: { fill: "#dcfce7", border: "#86efac", text: "#166534" },
            defense: { fill: "#dbeafe", border: "#93c5fd", text: "#1d4ed8" },
            midfield: { fill: "#fef3c7", border: "#fcd34d", text: "#92400e" },
            wide: { fill: "#ffedd5", border: "#fdba74", text: "#c2410c" },
            forward: { fill: "#fee2e2", border: "#fca5a5", text: "#b91c1c" },
            unknown: { fill: "#f1f5f9", border: "#cbd5e1", text: "#475569" }
        }
        return colors[group][shade]
    }

    readonly property string badgeText: normalizedLabel()
    readonly property string badgeGroup: groupForLabel(badgeText)

    radius: 6
    color: palette(badgeGroup, "fill")
    border.color: palette(badgeGroup, "border")
    implicitWidth: badgeLabel.implicitWidth + horizontalPadding * 2
    implicitHeight: 24

    Label {
        id: badgeLabel
        anchors.centerIn: parent
        text: root.badgeText
        font.bold: true
        font.pixelSize: 11
        color: root.palette(root.badgeGroup, "text")
    }
}

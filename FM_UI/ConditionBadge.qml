import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string label: ""
    property int value: 0
    property bool compact: false

    function conditionColor(score) {
        const normalizedLabel = (root.label || "").toLowerCase()
        if (normalizedLabel === "fit" || normalizedLabel === "fitness")
            return "#2fd06f"
        if (normalizedLabel === "m" || normalizedLabel === "mor" || normalizedLabel === "moral" || normalizedLabel === "morale")
            return "#f4c542"
        return "#f2b84b"
    }

    radius: 999
    color: Qt.rgba(255, 255, 255, 0.06)
    border.color: conditionColor(value)
    implicitWidth: compact ? 54 : 86
    implicitHeight: compact ? 22 : 24

    Label {
        anchors.centerIn: parent
        text: root.label + " " + root.value
        font.pixelSize: root.compact ? 10 : 11
        font.bold: true
        color: root.conditionColor(root.value)
    }
}

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string label: ""
    property int value: 0
    property bool compact: false
    property bool valueOnly: false
    property var metrics: null
    property real scaleFactor: metrics ? metrics.scale : 1.0

    function conditionColor(score) {
        if (score === undefined || score === null || isNaN(score)) {
            return "#64748b"
        }
        const normalizedScore = Math.max(0, Math.min(100, Number(score)))
        if (normalizedScore >= 80) {
            return "#22c55e"
        }
        if (normalizedScore >= 60) {
            return "#d9c53f"
        }
        if (normalizedScore >= 40) {
            return "#f97316"
        }
        return "#ef4444"
    }

    radius: 999
    color: Qt.rgba(255, 255, 255, 0.06)
    border.color: conditionColor(value)
    implicitWidth: Math.round((compact ? 54 : 86) * scaleFactor)
    implicitHeight: Math.round((compact ? 22 : 24) * scaleFactor)

    Label {
        anchors.centerIn: parent
        text: root.valueOnly ? String(root.value) : (root.label + " " + root.value)
        font.pixelSize: metrics ? metrics.font(root.compact ? 10 : 11) : Math.round((root.compact ? 10 : 11) * scaleFactor)
        font.bold: true
        color: root.conditionColor(root.value)
    }
}

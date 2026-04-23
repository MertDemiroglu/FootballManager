import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string label: ""
    property int value: 0

    function conditionColor(score) {
        const numeric = Number(score)
        if (numeric >= 75)
            return "#16a34a"
        if (numeric >= 50)
            return "#eab308"
        if (numeric >= 25)
            return "#f97316"
        return "#dc2626"
    }

    radius: 999
    color: Qt.rgba(0, 0, 0, 0.08)
    border.color: conditionColor(value)
    implicitWidth: 86
    implicitHeight: 24

    Label {
        anchors.centerIn: parent
        text: root.label + " " + root.value
        font.pixelSize: 11
        font.bold: true
        color: root.conditionColor(root.value)
    }
}

import QtQuick

Rectangle {
    id: root

    property int fieldMargin: 16
    property var metrics: null
    property real scaleFactor: metrics ? metrics.visualScale : 1.0
    property color pitchTopColor: "#0d5f37"
    property color pitchMidColor: "#073f28"
    property color pitchBottomColor: "#052c20"
    property color lineColor: "#a9d1b5"
    property alias fieldItem: pitchField

    radius: 12
    color: "#0b5c34"
    border.color: "#237447"
    border.width: Math.max(1, Math.round(1 * root.scaleFactor))
    clip: true

    gradient: Gradient {
        GradientStop { position: 0.0; color: root.pitchTopColor }
        GradientStop { position: 0.46; color: root.pitchMidColor }
        GradientStop { position: 1.0; color: root.pitchBottomColor }
    }

    Repeater {
        model: 8

        Rectangle {
            required property int index
            x: 0
            y: root.height * index / 8
            width: root.width
            height: root.height / 8
            color: index % 2 === 0 ? "#ffffff" : "#000000"
            opacity: index % 2 === 0 ? 0.025 : 0.05
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.width: Math.max(4, Math.round(12 * root.scaleFactor))
        border.color: "#031910"
        opacity: 0.22
    }

    Rectangle {
        id: pitchField
        anchors.fill: parent
        anchors.margins: Math.max(6, Math.round(root.fieldMargin * root.scaleFactor))
        radius: Math.round(10 * root.scaleFactor)
        color: "transparent"
        border.width: Math.max(1, Math.round(2 * root.scaleFactor))
        border.color: root.lineColor
        opacity: 0.62
    }

    Rectangle {
        anchors.left: pitchField.left
        anchors.right: pitchField.right
        anchors.verticalCenter: pitchField.verticalCenter
        height: Math.max(1, Math.round(2 * root.scaleFactor))
        color: root.lineColor
        opacity: 0.58
    }

    Rectangle {
        anchors.centerIn: pitchField
        width: Math.min(pitchField.width, pitchField.height) * 0.22
        height: width
        radius: width / 2
        color: "transparent"
        border.width: Math.max(1, Math.round(2 * root.scaleFactor))
        border.color: root.lineColor
        opacity: 0.56
    }

    Rectangle {
        anchors.horizontalCenter: pitchField.horizontalCenter
        anchors.top: pitchField.top
        width: pitchField.width * 0.42
        height: pitchField.height * 0.16
        color: "transparent"
        border.width: Math.max(1, Math.round(2 * root.scaleFactor))
        border.color: root.lineColor
        opacity: 0.54
    }

    Rectangle {
        anchors.horizontalCenter: pitchField.horizontalCenter
        anchors.bottom: pitchField.bottom
        width: pitchField.width * 0.42
        height: pitchField.height * 0.16
        color: "transparent"
        border.width: Math.max(1, Math.round(2 * root.scaleFactor))
        border.color: root.lineColor
        opacity: 0.54
    }

    Rectangle {
        anchors.horizontalCenter: pitchField.horizontalCenter
        anchors.top: pitchField.top
        width: pitchField.width * 0.22
        height: pitchField.height * 0.07
        color: "transparent"
        border.width: Math.max(1, Math.round(2 * root.scaleFactor))
        border.color: root.lineColor
        opacity: 0.5
    }

    Rectangle {
        anchors.horizontalCenter: pitchField.horizontalCenter
        anchors.bottom: pitchField.bottom
        width: pitchField.width * 0.22
        height: pitchField.height * 0.07
        color: "transparent"
        border.width: Math.max(1, Math.round(2 * root.scaleFactor))
        border.color: root.lineColor
        opacity: 0.5
    }
}

import QtQuick

Item {
    id: root

    property real maxCardWidth: 520
    property real cardRadius: 12
    property color overlayColor: "#66000000"
    property color cardColor: "#ffffff"
    property color cardBorderColor: "#d0d5dd"
    property real outerMargin: 48
    property real contentMargin: 20
    default property alias contentData: contentColumn.data

    Rectangle {
        anchors.fill: parent
        color: root.overlayColor
    }

    MouseArea {
        anchors.fill: parent
        onClicked: function(mouse) { mouse.accepted = true }
    }

    Rectangle {
        id: dialogCard
        width: Math.min(parent.width - root.outerMargin, root.maxCardWidth)
        implicitHeight: contentColumn.implicitHeight + (root.contentMargin * 2)
        height: implicitHeight
        anchors.centerIn: parent
        radius: root.cardRadius
        color: root.cardColor
        border.color: root.cardBorderColor

        Column {
            id: contentColumn
            anchors.top: parent.top
            anchors.topMargin: root.contentMargin
            anchors.left: parent.left
            anchors.leftMargin: root.contentMargin
            anchors.right: parent.right
            anchors.rightMargin: root.contentMargin
            spacing: 12
        }
    }
}

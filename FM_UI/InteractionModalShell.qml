import QtQuick
import QtQuick.Controls

Item {
    id: root

    signal overlayClicked()

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
        onClicked: function(mouse) {
            mouse.accepted = true
            root.overlayClicked()
        }
    }

    Rectangle {
        id: dialogCard
        readonly property real contentHeight: contentColumn.implicitHeight + (root.contentMargin * 2)
        width: Math.min(Math.max(0, parent.width - root.outerMargin), root.maxCardWidth)
        implicitHeight: Math.min(contentHeight, Math.max(0, parent.height - root.outerMargin))
        height: implicitHeight
        anchors.centerIn: parent
        radius: root.cardRadius
        color: root.cardColor
        border.color: root.cardBorderColor
        clip: true

        ScrollView {
            id: dialogScroll
            anchors.fill: parent
            anchors.margins: root.contentMargin
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Item {
                width: dialogScroll.availableWidth
                implicitHeight: contentColumn.implicitHeight

                Column {
                    id: contentColumn
                    width: parent.width
                    spacing: 12
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls

Item {
    id: root

    property real maxCardWidth: 520
    property real cardRadius: 12
    property color overlayColor: "#66000000"
    property color cardColor: "#ffffff"
    property color cardBorderColor: "#d0d5dd"
    property real outerMargin: 48
    property real contentMargin: 20
    property var metrics: null
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
        readonly property real safeOuterMargin: root.metrics ? root.metrics.px(root.metrics.dense ? 18 : root.outerMargin) : root.outerMargin
        readonly property real safeContentMargin: root.metrics ? root.metrics.cardPadding : root.contentMargin
        width: Math.max(1, Math.min(parent.width - safeOuterMargin * 2, root.maxCardWidth))
        implicitHeight: contentColumn.implicitHeight + (safeContentMargin * 2)
        height: Math.max(1, Math.min(implicitHeight, parent.height - safeOuterMargin * 2))
        anchors.centerIn: parent
        radius: root.metrics ? root.metrics.radiusLg : root.cardRadius
        color: root.cardColor
        border.color: root.cardBorderColor

        ScrollView {
            id: contentScroll
            anchors.fill: parent
            anchors.margins: dialogCard.safeContentMargin
            clip: true
            contentWidth: availableWidth
            contentHeight: contentColumn.implicitHeight
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Column {
                id: contentColumn
                width: contentScroll.availableWidth
                spacing: root.metrics ? root.metrics.spacingMd : 12
            }
        }
    }
}

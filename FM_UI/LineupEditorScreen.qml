import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    anchors.fill: parent
    color: "#f4f6fb"

    signal backRequested()

    function prepareLineup() {
        if (gameFacade) {
            gameFacade.ensureEditableLineupReady()
        }
    }

    Component.onCompleted: prepareLineup()
    onVisibleChanged: {
        if (visible) {
            prepareLineup()
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 20

        Rectangle {
            Layout.fillWidth: true
            radius: 18
            color: "#ffffff"
            border.color: "#d8dee8"
            implicitHeight: headerContent.implicitHeight + 24

            RowLayout {
                id: headerContent
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                Label {
                    Layout.fillWidth: true
                    text: "Lineup Editor"
                    font.pixelSize: 28
                    font.bold: true
                    color: "#17212f"
                }

                Button {
                    text: "Back"
                    Layout.preferredWidth: 108
                    Layout.preferredHeight: 40
                    onClicked: root.backRequested()
                }
            }
        }

        ScrollView {
            id: editorScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            contentHeight: editorContent.height

            LineupEditorView {
                id: editorContent
                width: editorScroll.availableWidth
                height: Math.max(editorScroll.availableHeight, implicitHeight)
            }
        }
    }
}

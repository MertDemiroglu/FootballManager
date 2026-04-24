import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    property var gameFacade

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
    onGameFacadeChanged: prepareLineup()

    Rectangle {
        anchors.fill: parent
        color: "#f4f6fb"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 20

            Rectangle {
                Layout.fillWidth: true
                radius: 18
                color: "#ffffff"
                border.color: "#d8dee8"
                implicitHeight: headerContent.implicitHeight + 32

                RowLayout {
                    id: headerContent
                    anchors.fill: parent
                    anchors.margins: 16
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
                        Layout.preferredHeight: 44
                        onClicked: root.backRequested()
                    }
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                LineupEditorView {
                    width: parent.width
                    gameFacade: root.gameFacade
                    readOnly: false
                }
            }
        }
    }
}

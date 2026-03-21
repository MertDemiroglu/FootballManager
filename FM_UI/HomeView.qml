import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    signal newGameRequested()
    signal quitRequested()

    Rectangle {
        anchors.fill: parent
        color: "#f5f5f5"

        Rectangle {
            width: Math.min(parent.width - 48, 720)
            anchors.centerIn: parent
            radius: 16
            color: "white"
            border.color: "#d8d8d8"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 32
                spacing: 20

                Label {
                    text: "Football Manager"
                    font.pixelSize: 34
                    font.bold: true
                    color: "#202020"
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "Start a new career, pick your club, and continue to the dashboard when the session is ready."
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    color: "#5a5a5a"
                    Layout.fillWidth: true
                }

                Item { Layout.preferredHeight: 8 }

                Button {
                    text: "New Game"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    onClicked: root.newGameRequested()
                }

                Button {
                    text: "Continue"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    enabled: false
                }

                Button {
                    text: "Quit"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    onClicked: root.quitRequested()
                }
            }
        }
    }
}
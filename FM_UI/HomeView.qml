import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    signal newGameRequested()
    signal quitRequested()

    Rectangle {
        anchors.fill: parent
        color: "#f4f6fb"

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(Math.max(parent.width - 48, 520), 680)
            height: Math.min(Math.max(parent.height * 0.56, 340), 460)
            radius: 20
            color: "#ffffff"
            border.color: "#d8dee8"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 36
                spacing: 20

                Item { Layout.fillHeight: true }

                Label {
                    text: "Football Manager"
                    font.pixelSize: 38
                    font.bold: true
                    color: "#17212f"
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    text: "Start a new career, choose your club, and manage the season from a clean prototype dashboard."
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    color: "#526071"
                    font.pixelSize: 17
                    Layout.fillWidth: true
                }

                Item { Layout.preferredHeight: 4 }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 14

                    Button {
                        text: "New Game"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        font.pixelSize: 16
                        onClicked: root.newGameRequested()
                    }

                    Button {
                        text: "Continue"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        font.pixelSize: 16
                        enabled: false
                    }

                    Button {
                        text: "Quit"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        font.pixelSize: 16
                        onClicked: root.quitRequested()
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}

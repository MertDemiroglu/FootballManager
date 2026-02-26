import QtQuick
import QtQuick.Controls
import FM_UI

ApplicationWindow {
    id: root
    visible: true
    width: 1000
    height: 1000
    title: "Mini Football Manager"

    Column {
        anchors.centerIn: parent
        spacing: 12

        Text {
            text: "UI State: " + gameController.uiState
            font.pixelSize: 18
        }

        Button {
            text: "New Game"
            onClicked: gameController.startNewGame()
        }
    }
}

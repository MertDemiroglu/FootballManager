import QtQuick
import QtQuick.Controls
import FM_UI

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 600
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

        Button {
            text: "Confirm User (Test)"
            onClicked: gameController.confirmUser("Mert", "Test FC")
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    signal newGameRequested()
    signal continueRequested()
    signal loadGameRequested(string saveSlotId)
    signal quitRequested()

    property bool hasContinueSave: false
    property bool showLoadGame: false
    property var saveSlots: []

    function refreshSaveSlots() {
        hasContinueSave = gameFacade.hasContinueSave()
        saveSlots = gameFacade.listSaveSlots()
    }

    function managedClubText(saveSlot) {
        if (saveSlot.managedTeamName) {
            return saveSlot.managedTeamName
        }
        return "Managed club unavailable"
    }

    function saveCardTitle(saveSlot) {
        return (saveSlot.managerName || "Manager") + " - " + root.managedClubText(saveSlot)
    }

    function formattedGameDate(saveSlot) {
        var value = saveSlot.currentDate || ""
        if (!value) {
            return "Unknown"
        }
        var parts = value.split("-")
        if (parts.length !== 3) {
            return value
        }
        var parsed = new Date(Number(parts[0]), Number(parts[1]) - 1, Number(parts[2]))
        if (isNaN(parsed.getTime())) {
            return value
        }
        return Qt.formatDate(parsed, "MMMM d, yyyy")
    }

    function formattedTimestamp(value) {
        if (!value) {
            return "unknown"
        }
        var parsed = new Date(value)
        if (isNaN(parsed.getTime())) {
            return value
        }
        return Qt.formatDateTime(parsed, "d MMM yyyy HH:mm")
    }

    Component.onCompleted: refreshSaveSlots()

    Rectangle {
        anchors.fill: parent
        color: "#f4f6fb"

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(Math.max(parent.width - 48, 520), 680)
            height: Math.min(Math.max(parent.height * 0.68, 430), 640)
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
                    text: "Start a new career, choose your club, and manage the season from a clean manager dashboard."
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
                        enabled: root.hasContinueSave
                        onClicked: root.continueRequested()
                    }

                    Button {
                        text: "Load Game"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        font.pixelSize: 16
                        onClicked: {
                            if (gameFacade.lastError) {
                                gameFacade.clearLastError()
                            }
                            root.showLoadGame = !root.showLoadGame
                            if (root.showLoadGame) {
                                root.refreshSaveSlots()
                            }
                        }
                    }

                    Button {
                        text: "Quit"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        font.pixelSize: 16
                        onClicked: root.quitRequested()
                    }
                }

                ColumnLayout {
                    visible: root.showLoadGame
                    Layout.fillWidth: true
                    Layout.maximumHeight: 190
                    spacing: 8

                    Label {
                        text: root.saveSlots.length > 0 ? "Load Game" : "No saves found"
                        font.pixelSize: 15
                        font.bold: true
                        color: "#17212f"
                        Layout.fillWidth: true
                    }

                    ListView {
                        visible: root.saveSlots.length > 0
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 8
                        model: root.saveSlots

                        delegate: Rectangle {
                            required property var modelData
                            width: ListView.view.width
                            height: 78
                            radius: 10
                            color: "#f8fafc"
                            border.color: "#d8dee8"

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 3

                                    Label {
                                        text: root.saveCardTitle(modelData)
                                        font.pixelSize: 15
                                        font.bold: true
                                        color: "#17212f"
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        text: "Game Date: " + root.formattedGameDate(modelData)
                                        color: "#526071"
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        text: "Created: " + root.formattedTimestamp(modelData.createdAtUtc)
                                              + " \u2022 Updated: " + root.formattedTimestamp(modelData.updatedAtUtc)
                                        color: "#667085"
                                        font.pixelSize: 12
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }

                                Button {
                                    text: "Load"
                                    Layout.preferredHeight: 36
                                    onClicked: root.loadGameRequested(modelData.saveSlotId || "")
                                }
                            }
                        }
                    }
                }

                Label {
                    visible: !!gameFacade.lastError
                    text: gameFacade.lastError
                    color: "#b3261e"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    Connections {
        target: gameFacade
        function onGameStateChanged() {
            root.refreshSaveSlots()
        }
    }
}

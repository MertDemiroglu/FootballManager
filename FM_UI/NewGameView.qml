import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    signal gameStarted()

    property var teamList: []
    property int selectedTeamId: -1
    property string selectedTeamName: ""

    function refreshData() {
        teamList = gameFacade.getTeamSelectionList()
        if (teamList.length > 0) {
            var stillSelected = false
            for (var i = 0; i < teamList.length; ++i) {
                if (teamList[i].teamId === selectedTeamId) {
                    stillSelected = true
                    selectedTeamName = teamList[i].teamName || ""
                    break
                }
            }
            if (!stillSelected) {
                selectedTeamId = teamList[0].teamId
                selectedTeamName = teamList[0].teamName || ""
            }
        } else {
            selectedTeamId = -1
            selectedTeamName = ""
        }
    }

    Component.onCompleted: refreshData()

    Connections {
        target: gameFacade

        function onGameStateChanged() {
            refreshData()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#f5f5f5"

        Rectangle {
            width: Math.min(parent.width - 48, 720)
            height: Math.min(parent.height - 48, 760)
            anchors.centerIn: parent
            radius: 10
            color: "white"
            border.color: "#d8d8d8"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 18

                Label {
                    text: "Start New Game"
                    font.pixelSize: 28
                    font.bold: true
                    color: "#202020"
                }

                Label {
                    text: "Choose a team and enter a manager name to begin."
                    color: "#5a5a5a"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    Label {
                        text: "Manager Name"
                        font.bold: true
                    }

                    TextField {
                        id: managerNameField
                        Layout.fillWidth: true
                        placeholderText: "Manager name"
                    }
                }

                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Label {
                        text: "Team Selection"
                        font.bold: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: "#fafafa"
                        border.color: "#dddddd"

                        ListView {
                            id: teamListView
                            anchors.fill: parent
                            anchors.margins: 8
                            clip: true
                            spacing: 6
                            model: root.teamList

                            delegate: Rectangle {
                                required property var modelData
                                width: ListView.view.width
                                height: 64
                                radius: 8
                                color: modelData.teamId === root.selectedTeamId ? "#e6f0ff" : "white"
                                border.color: modelData.teamId === root.selectedTeamId ? "#5b8def" : "#d6d6d6"

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        root.selectedTeamId = parent.modelData.teamId
                                        root.selectedTeamName = parent.modelData.teamName || ""
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 12

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4

                                        Label {
                                            text: modelData.teamName || "Unnamed Team"
                                            font.pixelSize: 16
                                            font.bold: true
                                            color: "#202020"
                                        }

                                        Label {
                                            text: (modelData.shortRatingSummary || "") + "   Squad: " + (modelData.squadSize || 0)
                                            color: "#666666"
                                        }
                                    }

                                    Label {
                                        text: modelData.teamId === root.selectedTeamId ? "Selected" : ""
                                        color: "#2c5cc5"
                                        font.bold: true
                                    }
                                }
                            }

                            footer: Item {
                                width: teamListView.width
                                height: root.teamList.length === 0 ? 80 : 0

                                Label {
                                    anchors.centerIn: parent
                                    visible: root.teamList.length === 0
                                    text: "No data"
                                    color: "#666666"
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Label {
                        Layout.fillWidth: true
                        text: root.selectedTeamId > 0
                              ? "Selected team: " + root.selectedTeamName
                              : "Select a team to continue"
                        color: "#444444"
                    }

                    Button {
                        text: "Start Game"
                        enabled: root.selectedTeamId > 0
                        onClicked: {
                            var managerName = managerNameField.text
                            if (gameFacade.startNewGame(root.selectedTeamId, managerName)) {
                                root.gameStarted()
                            }
                        }
                    }
                }
            }
        }
    }
}
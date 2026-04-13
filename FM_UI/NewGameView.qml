import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    signal backRequested()
    signal gameStarted()

    property var teamList: []
    property int selectedLeagueId: -1
    property int selectedTeamId: -1
    property string selectedTeamName: ""

    function refreshData() {
        teamList = gameFacade.getTeamSelectionList()
        if (teamList.length > 0) {
            var stillSelected = false
            for (var i = 0; i < teamList.length; ++i) {
                if (teamList[i].teamId === selectedTeamId) {
                    stillSelected = true
                     selectedLeagueId = teamList[i].leagueId || -1
                    selectedTeamName = teamList[i].teamName || ""
                    break
                }
            }
            if (!stillSelected) {
                selectedLeagueId = teamList[0].leagueId || -1
                selectedTeamId = teamList[0].teamId
                selectedTeamName = teamList[0].teamName || ""
            }
        } else {
            selectedLeagueId = -1
            selectedTeamId = -1
            selectedTeamName = ""
        }
    }

    function trimmedManagerName() {
        return managerNameField.text.trim()
    }

    Component.onCompleted: refreshData()

    Rectangle {
        anchors.fill: parent
        color: "#f5f5f5"

        Item {
            anchors.fill: parent
            anchors.margins: 24

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width, 640)
                height: Math.min(parent.height, 760)
                radius: 16
                color: "#ffffff"
                border.color: "#d9dee7"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 28
                    spacing: 18

                    Label {
                        text: "Start New Game"
                        font.pixelSize: 30
                        font.bold: true
                        color: "#1f2937"
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "Choose a club and enter a manager name to start the career flow."
                        color: "#5f6b7a"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Manager Name"
                            font.pixelSize: 14
                            font.bold: true
                            color: "#1f2937"
                        }

                        TextField {
                            id: managerNameField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 42
                            placeholderText: "Manager name"
                            onTextChanged: {
                                if (gameFacade.lastError) {
                                    gameFacade.clearLastError()
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 8

                        Label {
                            text: "Team Selection"
                            font.pixelSize: 14
                            font.bold: true
                            color: "#1f2937"
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 320
                            radius: 12
                            color: "#f8fafc"
                            border.color: "#d9dee7"

                            ListView {
                                id: teamListView
                                anchors.fill: parent
                                anchors.margins: 10
                                clip: true
                                spacing: 8
                                model: root.teamList

                                delegate: Rectangle {
                                    required property var modelData
                                    width: ListView.view.width
                                    height: 76
                                    radius: 10
                                    color: modelData.teamId === root.selectedTeamId ? "#e8f0ff" : "#ffffff"
                                    border.color: modelData.teamId === root.selectedTeamId ? "#5b8def" : "#d6dde8"

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            root.selectedTeamId = parent.modelData.teamId
                                            root.selectedLeagueId = parent.modelData.leagueId || -1
                                            root.selectedTeamName = parent.modelData.teamName || ""
                                            if (gameFacade.lastError) {
                                                gameFacade.clearLastError()
                                            }
                                        }
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 14
                                        spacing: 12

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 4

                                            Label {
                                                text: modelData.teamName || "Unnamed Team"
                                                font.pixelSize: 17
                                                font.bold: true
                                                color: "#1f2937"
                                                Layout.fillWidth: true
                                                elide: Text.ElideRight
                                            }

                                            Label {
                                                text: (modelData.shortRatingSummary || "No rating summary") + " • Squad: " + (modelData.squadSize || 0)
                                                color: "#667085"
                                                Layout.fillWidth: true
                                                elide: Text.ElideRight
                                            }
                                        }

                                        Rectangle {
                                            visible: modelData.teamId === root.selectedTeamId
                                            Layout.alignment: Qt.AlignVCenter
                                            radius: 999
                                            color: "#dbe8ff"
                                            border.color: "#8fb0f5"
                                            implicitWidth: selectedLabel.implicitWidth + 18
                                            implicitHeight: 28

                                            Label {
                                                id: selectedLabel
                                                anchors.centerIn: parent
                                                text: "Selected"
                                                color: "#2f5fbf"
                                                font.bold: true
                                            }
                                        }
                                    }
                                }

                                footer: Item {
                                    width: teamListView.width
                                    height: root.teamList.length === 0 ? 120 : 0

                                    Label {
                                        anchors.centerIn: parent
                                        visible: root.teamList.length === 0
                                        text: "No teams are available right now."
                                        color: "#667085"
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
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Button {
                            text: "Back"
                            Layout.preferredHeight: 42
                            onClicked: root.backRequested()
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.selectedTeamId > 0
                                  ? "Selected team: " + root.selectedTeamName
                                  : "Select a team to continue"
                            color: "#475467"
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        Button {
                            text: "Start Game"
                            Layout.preferredHeight: 42
                            enabled: root.selectedTeamId > 0
                                     && root.selectedLeagueId > 0
                                     && root.trimmedManagerName().length > 0
                                     && root.teamList.length > 0
                            onClicked: {
                                var didStart = gameFacade.startNewGameForLeagueTeam(
                                            root.selectedLeagueId,
                                            root.selectedTeamId,
                                            root.trimmedManagerName())
                                if (didStart) {
                                    root.gameStarted()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    signal backRequested()

    readonly property var shellState: gameFacade.shellState
    readonly property bool hasActiveGame: shellState.hasStartedGame

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "Standings"
                font.pixelSize: 28
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Back"
                onClicked: root.backRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 8
            visible: root.hasActiveGame
            color: "#eef1f5"
            border.color: "#d2d8df"
            implicitHeight: 42

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Repeater {
                    model: [
                        ["Pos", 50], ["Team", 230], ["P", 50], ["W", 50], ["D", 50],
                        ["L", 50], ["GF", 60], ["GA", 60], ["GD", 60], ["Pts", 60]
                    ]

                    delegate: Label {
                        required property var modelData
                        Layout.preferredWidth: modelData[1]
                        text: modelData[0]
                        font.bold: true
                        color: "#203040"
                    }
                }
            }
        }

        Label {
            visible: !root.hasActiveGame
            text: "No active game. Start a new game to view standings."
            color: "#666666"
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.hasActiveGame
            clip: true

            Column {
                width: root.width
                spacing: 6

                Repeater {
                    model: gameFacade.standingsModel

                    delegate: Rectangle {
                        width: parent.width
                        height: 44
                        radius: 8
                        color: isSelectedTeam ? "#e6f0ff" : "white"
                        border.color: isSelectedTeam ? "#5b8def" : "#d8d8d8"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            Label { Layout.preferredWidth: 50; text: position || "-" }
                            Label { Layout.preferredWidth: 230; text: teamName || "-"; elide: Text.ElideRight }
                            Label { Layout.preferredWidth: 50; text: played || 0 }
                            Label { Layout.preferredWidth: 50; text: wins || 0 }
                            Label { Layout.preferredWidth: 50; text: draws || 0 }
                            Label { Layout.preferredWidth: 50; text: losses || 0 }
                            Label { Layout.preferredWidth: 60; text: goalsFor || 0 }
                            Label { Layout.preferredWidth: 60; text: goalsAgainst || 0 }
                            Label { Layout.preferredWidth: 60; text: goalDifference || 0 }
                            Label { Layout.preferredWidth: 60; text: points || 0; font.bold: true }
                        }
                    }
                }

                Label {
                    visible: gameFacade.standingsModel.rowCount() === 0
                    text: "No data"
                    color: "#666666"
                }
            }
        }
    }
}

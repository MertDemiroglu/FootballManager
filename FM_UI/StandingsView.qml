import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    signal backRequested()

    property var standingsTable: []

    function refreshData() {
        standingsTable = gameFacade.getStandingsTable()
    }

    Component.onCompleted: refreshData()

    Connections {
        target: gameFacade

        function onGameStateChanged() {
            refreshData()
        }
    }

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

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Column {
                width: root.width
                spacing: 6

                Repeater {
                    model: root.standingsTable

                    delegate: Rectangle {
                        required property var modelData
                        width: parent.width
                        height: 44
                        radius: 8
                        color: modelData.teamId === gameFacade.getSelectedTeamId() ? "#e6f0ff" : "white"
                        border.color: modelData.teamId === gameFacade.getSelectedTeamId() ? "#5b8def" : "#d8d8d8"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            Label { Layout.preferredWidth: 50; text: modelData.position || "-" }
                            Label { Layout.preferredWidth: 230; text: modelData.teamName || "-"; elide: Text.ElideRight }
                            Label { Layout.preferredWidth: 50; text: modelData.played || 0 }
                            Label { Layout.preferredWidth: 50; text: modelData.wins || 0 }
                            Label { Layout.preferredWidth: 50; text: modelData.draws || 0 }
                            Label { Layout.preferredWidth: 50; text: modelData.losses || 0 }
                            Label { Layout.preferredWidth: 60; text: modelData.goalsFor || 0 }
                            Label { Layout.preferredWidth: 60; text: modelData.goalsAgainst || 0 }
                            Label { Layout.preferredWidth: 60; text: modelData.goalDifference || 0 }
                            Label { Layout.preferredWidth: 60; text: modelData.points || 0; font.bold: true }
                        }
                    }
                }

                Label {
                    visible: root.standingsTable.length === 0
                    text: "No data"
                    color: "#666666"
                }
            }
        }
    }
}
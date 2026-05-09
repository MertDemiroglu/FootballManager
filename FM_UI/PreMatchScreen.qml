import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var interactionData: ({})
    property string selectedTeamName: ""
    property string currentDateText: ""

    signal backRequested()
    signal editLineupRequested()
    signal playMatchRequested()

    readonly property color backgroundColor: "#071016"
    readonly property color shellColor: "#08111a"
    readonly property color panelColor: "#0f1a24"
    readonly property color borderColor: "#243442"
    readonly property color textPrimary: "#f5f7fa"
    readonly property color textSecondary: "#a8b3c1"
    readonly property color green: "#22c55e"

    function hasMatchData() {
        return interactionData && interactionData.hasData !== false && (interactionData.homeTeamName || interactionData.awayTeamName)
    }

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 74
                color: root.shellColor
                border.color: root.borderColor

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 28
                    anchors.rightMargin: 28
                    spacing: 20

                    MatchTeamBadge {
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44
                        badgeSize: 44
                        teamName: root.selectedTeamName || interactionData.homeTeamName || "No Team"
                    }

                    Label {
                        text: root.selectedTeamName || "No Team"
                        color: root.textPrimary
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Rectangle {
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: 38
                        color: root.green
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Pre-Match"
                        color: root.textPrimary
                        font.pixelSize: 24
                        font.bold: true
                    }

                    Label {
                        text: root.currentDateText || interactionData.dateText || ""
                        color: root.textSecondary
                        font.pixelSize: 16
                    }

                    MatchFlowButton {
                        Layout.preferredWidth: 126
                        Layout.preferredHeight: 44
                        text: "Back"
                        iconName: "main-menu"
                        primary: false
                        onClicked: root.backRequested()
                    }
                }
            }

            Loader {
                Layout.fillWidth: true
                Layout.fillHeight: true
                sourceComponent: root.hasMatchData() ? matchContent : fallbackContent
            }
        }
    }

    Component {
        id: fallbackContent

        Item {
            Column {
                anchors.centerIn: parent
                width: Math.min(parent.width - 64, 560)
                spacing: 18

                Label {
                    width: parent.width
                    text: "No pre-match interaction is active."
                    color: root.textPrimary
                    font.pixelSize: 26
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }

                MatchFlowButton {
                    width: 190
                    height: 48
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Back to Dashboard"
                    primary: true
                    onClicked: root.backRequested()
                }
            }
        }
    }

    Component {
        id: matchContent

        Item {
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 30
                anchors.rightMargin: 30
                anchors.topMargin: 24
                anchors.bottomMargin: 22
                spacing: 20

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 132
                    spacing: 28

                    TeamHero {
                        Layout.fillWidth: true
                        teamName: interactionData.homeTeamName || "Home"
                        sideText: "Home"
                        alignRight: true
                    }

                    Column {
                        Layout.preferredWidth: 260
                        spacing: 14

                        Label {
                            width: parent.width
                            text: "VS"
                            color: root.textPrimary
                            font.pixelSize: 42
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 14
                            Label { text: interactionData.dateText || ""; color: root.textSecondary; font.pixelSize: 15 }
                            Rectangle { width: 5; height: 5; radius: 3; color: root.green; anchors.verticalCenter: parent.verticalCenter }
                            Label { text: "Matchweek " + (interactionData.matchweek || "-"); color: root.textSecondary; font.pixelSize: 15 }
                        }
                    }

                    TeamHero {
                        Layout.fillWidth: true
                        teamName: interactionData.awayTeamName || "Away"
                        sideText: "Away"
                        alignRight: false
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 28

                    PitchPanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        label: "HOME LINEUP"
                        teamName: interactionData.homeTeamName || "Home"
                        formationText: interactionData.homeFormationText || "-"
                        lineupRows: interactionData.homeLineup || []
                        kitPrimary: "#f97316"
                        kitSecondary: "#22c55e"
                    }

                    PitchPanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        label: "AWAY LINEUP"
                        teamName: interactionData.awayTeamName || "Away"
                        formationText: interactionData.awayFormationText || "-"
                        lineupRows: interactionData.awayLineup || []
                        kitPrimary: "#1d4ed8"
                        kitSecondary: "#f97316"
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 18

                    Item { Layout.fillWidth: true }

                    MatchFlowButton {
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: 58
                        text: "Edit Lineup"
                        iconName: "lineup"
                        primary: false
                        onClicked: root.editLineupRequested()
                    }

                    MatchFlowButton {
                        Layout.preferredWidth: 260
                        Layout.preferredHeight: 58
                        text: "Play Match"
                        iconName: "play"
                        primary: true
                        onClicked: root.playMatchRequested()
                    }
                }
            }
        }
    }

    component TeamHero: Item {
        id: teamHeroRoot
        property string teamName: ""
        property string sideText: ""
        property bool alignRight: false

        RowLayout {
            anchors.fill: parent
            layoutDirection: alignRight ? Qt.RightToLeft : Qt.LeftToRight
            spacing: 22

            MatchTeamBadge {
                Layout.preferredWidth: 92
                Layout.preferredHeight: 92
                badgeSize: 92
                teamName: teamHeroRoot.teamName
            }

            Column {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    width: parent.width
                    text: teamHeroRoot.teamName
                    color: root.textPrimary
                    font.pixelSize: 30
                    font.bold: true
                    horizontalAlignment: alignRight ? Text.AlignRight : Text.AlignLeft
                    elide: Text.ElideRight
                }

                Label {
                    width: parent.width
                    text: teamHeroRoot.sideText
                    color: teamHeroRoot.sideText === "Home" ? root.green : "#38bdf8"
                    font.pixelSize: 17
                    horizontalAlignment: alignRight ? Text.AlignRight : Text.AlignLeft
                }
            }
        }
    }

    component PitchPanel: Rectangle {
        id: pitchPanelRoot
        property string label: ""
        property string teamName: ""
        property string formationText: ""
        property var lineupRows: []
        property string kitPrimary: "#f97316"
        property string kitSecondary: "#22c55e"

        radius: 12
        color: root.panelColor
        border.color: root.borderColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            RowLayout {
                Layout.fillWidth: true

                AppIcon {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    name: "lineup"
                    size: 26
                    opacity: 0.95
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 2
                    Label { text: label; color: root.green; font.pixelSize: 13; font.bold: true }
                    Label { text: teamName; color: root.textPrimary; font.pixelSize: 22; font.bold: true; elide: Text.ElideRight; width: parent.width }
                }

                Column {
                    spacing: 4
                    Label { text: "Formation: " + (formationText || "-"); color: root.textSecondary; font.pixelSize: 14; horizontalAlignment: Text.AlignRight; width: 160 }
                    Label { text: "Avg XI OVR -"; color: root.textSecondary; font.pixelSize: 14; horizontalAlignment: Text.AlignRight; width: 160 }
                }
            }

            MatchPitchPreview {
                Layout.fillWidth: true
                Layout.fillHeight: true
                lineupRows: pitchPanelRoot.lineupRows
                formationText: pitchPanelRoot.formationText
                mode: "preMatch"
                kitColorPrimary: pitchPanelRoot.kitPrimary
                kitColorSecondary: pitchPanelRoot.kitSecondary
            }
        }
    }

    component MatchFlowButton: Rectangle {
        id: buttonRoot
        signal clicked()
        property string text: ""
        property string iconName: ""
        property bool primary: false

        radius: 8
        color: primary
               ? (mouse.pressed ? Qt.darker(root.green, 1.25) : (mouse.containsMouse ? Qt.lighter(root.green, 1.08) : root.green))
               : (mouse.containsMouse ? "#10291d" : "#0b1118")
        border.color: primary ? Qt.lighter(root.green, 1.1) : root.borderColor

        Row {
            anchors.centerIn: parent
            spacing: 12

            AppIcon {
                visible: buttonRoot.iconName.length > 0
                name: buttonRoot.iconName
                size: 22
                anchors.verticalCenter: parent.verticalCenter
            }

            Label {
                text: buttonRoot.text
                color: root.textPrimary
                font.pixelSize: 17
                font.bold: primary
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: buttonRoot.clicked()
        }
    }
}

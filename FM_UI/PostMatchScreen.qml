import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var interactionData: ({})
    property string selectedTeamName: ""
    property string selectedTeamPrimaryColor: "#22c55e"
    property string selectedTeamSecondaryColor: "#0f172a"
    property string selectedTeamTextColor: "#f8fafc"
    property string currentDateText: ""

    signal continueRequested()
    signal viewDetailsRequested(var matchId)
    signal backRequested()

    readonly property color backgroundColor: "#071016"
    readonly property color shellColor: "#08111a"
    readonly property color panelColor: "#0f1a24"
    readonly property color borderColor: "#243442"
    readonly property color textPrimary: "#f5f7fa"
    readonly property color textSecondary: "#a8b3c1"
    readonly property color green: "#22c55e"
    readonly property color awayAccent: "#f97316"

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
                Layout.preferredHeight: 78
                color: root.shellColor
                border.color: root.borderColor

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 28
                    anchors.rightMargin: 28
                    spacing: 16

                    MatchTeamBadge {
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44
                        badgeSize: 44
                        teamName: root.selectedTeamName || interactionData.homeTeamName || "No Team"
                        primaryColor: root.selectedTeamPrimaryColor
                        secondaryColor: root.selectedTeamSecondaryColor
                        textColor: root.selectedTeamTextColor
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.selectedTeamName || "No Team"
                        color: root.textPrimary
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Column {
                        Layout.preferredWidth: 360
                        spacing: 3
                        Label {
                            width: parent.width
                            text: "Match Finished"
                            color: root.textPrimary
                            font.pixelSize: 27
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }
                        Label {
                            width: parent.width
                            text: "The match has ended"
                            color: root.textSecondary
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.currentDateText || interactionData.dateText || ""
                        color: root.textSecondary
                        font.pixelSize: 16
                        horizontalAlignment: Text.AlignRight
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
                    text: "No post-match interaction is active."
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
                anchors.leftMargin: 28
                anchors.rightMargin: 28
                anchors.topMargin: 30
                anchors.bottomMargin: 20
                spacing: 38

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 144
                    spacing: 28

                    ScoreTeamHero {
                        Layout.fillWidth: true
                        teamName: interactionData.homeTeamName || "Home"
                        sideText: "Home"
                        alignRight: true
                        primaryColor: interactionData.homePrimaryColor || "#22c55e"
                        secondaryColor: interactionData.homeSecondaryColor || "#0f172a"
                        badgeTextColor: interactionData.homeTextColor || "#f8fafc"
                        accentColor: interactionData.homePrimaryColor || root.green
                    }

                    Column {
                        Layout.preferredWidth: 360
                        spacing: 12

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 30

                            Label {
                                text: interactionData.homeGoals !== undefined ? interactionData.homeGoals : "-"
                                color: root.textPrimary
                                font.pixelSize: 64
                                font.bold: true
                            }

                            Label {
                                text: "-"
                                color: root.textPrimary
                                font.pixelSize: 52
                                font.bold: true
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Label {
                                text: interactionData.awayGoals !== undefined ? interactionData.awayGoals : "-"
                                color: root.textPrimary
                                font.pixelSize: 64
                                font.bold: true
                            }
                        }

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 14
                            Label { text: interactionData.dateText || ""; color: root.textSecondary; font.pixelSize: 15 }
                            Rectangle { width: 5; height: 5; radius: 3; color: root.green; anchors.verticalCenter: parent.verticalCenter }
                            Label { text: "Matchweek " + (interactionData.matchweek || "-"); color: root.textSecondary; font.pixelSize: 15 }
                        }
                    }

                    ScoreTeamHero {
                        Layout.fillWidth: true
                        teamName: interactionData.awayTeamName || "Away"
                        sideText: "Away"
                        alignRight: false
                        primaryColor: interactionData.awayPrimaryColor || "#22c55e"
                        secondaryColor: interactionData.awaySecondaryColor || "#0f172a"
                        badgeTextColor: interactionData.awayTextColor || "#f8fafc"
                        accentColor: interactionData.awayPrimaryColor || "#38bdf8"
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 18

                    PitchPanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        teamName: interactionData.homeTeamName || "Home"
                        formationText: interactionData.homeFormationText || "-"
                        lineupRows: interactionData.homeLineup || []
                        averageText: "Avg XI OVR " + (interactionData.homeAverageOverallText || "--")
                        kitPrimary: interactionData.homePrimaryColor || "#22c55e"
                        kitSecondary: interactionData.homeSecondaryColor || "#0f172a"
                    }

                    MatchStatsPanel {
                        Layout.preferredWidth: 430
                        Layout.fillHeight: true
                    }

                    PitchPanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        teamName: interactionData.awayTeamName || "Away"
                        formationText: interactionData.awayFormationText || "-"
                        lineupRows: interactionData.awayLineup || []
                        averageText: "Avg XI OVR " + (interactionData.awayAverageOverallText || "--")
                        kitPrimary: interactionData.awayPrimaryColor || "#22c55e"
                        kitSecondary: interactionData.awaySecondaryColor || "#0f172a"
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 18

                    Item { Layout.fillWidth: true }

                    MatchFlowButton {
                        Layout.preferredWidth: 260
                        Layout.preferredHeight: 58
                        text: "Open Match Report"
                        iconName: "record"
                        primary: false
                        enabled: (interactionData.matchId || 0) > 0
                        onClicked: root.viewDetailsRequested(interactionData.matchId || 0)
                    }

                    MatchFlowButton {
                        Layout.preferredWidth: 260
                        Layout.preferredHeight: 58
                        text: "Continue"
                        iconName: "chevron-right"
                        primary: true
                        onClicked: root.continueRequested()
                    }

                    Item { Layout.fillWidth: true }
                }
            }
        }
    }

    component ScoreTeamHero: Item {
        id: scoreTeamHeroRoot
        property string teamName: ""
        property string sideText: ""
        property bool alignRight: false
        property string primaryColor: "#22c55e"
        property string secondaryColor: "#0f172a"
        property string badgeTextColor: "#f8fafc"
        property color accentColor: root.green

        RowLayout {
            anchors.fill: parent
            layoutDirection: scoreTeamHeroRoot.alignRight ? Qt.RightToLeft : Qt.LeftToRight
            spacing: 22

            MatchTeamBadge {
                Layout.preferredWidth: 92
                Layout.preferredHeight: 92
                badgeSize: 92
                teamName: scoreTeamHeroRoot.teamName
                primaryColor: scoreTeamHeroRoot.primaryColor
                secondaryColor: scoreTeamHeroRoot.secondaryColor
                textColor: scoreTeamHeroRoot.badgeTextColor
            }

            Column {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    width: parent.width
                    text: scoreTeamHeroRoot.teamName
                    color: root.textPrimary
                    font.pixelSize: 30
                    font.bold: true
                    horizontalAlignment: scoreTeamHeroRoot.alignRight ? Text.AlignRight : Text.AlignLeft
                    elide: Text.ElideRight
                }

                Rectangle {
                    width: 48
                    height: 2
                    color: scoreTeamHeroRoot.accentColor
                    x: scoreTeamHeroRoot.alignRight ? parent.width - width : 0
                }

                Label {
                    width: parent.width
                    text: scoreTeamHeroRoot.sideText
                    color: scoreTeamHeroRoot.accentColor
                    font.pixelSize: 17
                    horizontalAlignment: scoreTeamHeroRoot.alignRight ? Text.AlignRight : Text.AlignLeft
                }
            }
        }
    }

    component PitchPanel: Rectangle {
        id: pitchPanelRoot
        property string teamName: ""
        property string formationText: ""
        property var lineupRows: []
        property string averageText: ""
        property string kitPrimary: "#f97316"
        property string kitSecondary: "#22c55e"

        radius: 12
        color: root.panelColor
        border.color: root.borderColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 16

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 52

                Column {
                    Layout.fillWidth: true
                    spacing: 6
                    Label { text: pitchPanelRoot.teamName; color: root.textPrimary; font.pixelSize: 19; font.bold: true; elide: Text.ElideRight; width: parent.width }
                    Label { text: "Formation: " + (pitchPanelRoot.formationText || "-"); color: root.textSecondary; font.pixelSize: 14 }
                }

                Label {
                    text: pitchPanelRoot.averageText
                    color: root.textSecondary
                    font.pixelSize: 14
                }
            }

            MatchPitchPreview {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: 12
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Layout.bottomMargin: 8
                lineupRows: pitchPanelRoot.lineupRows
                formationText: pitchPanelRoot.formationText
                mode: "postMatch"
                kitColorPrimary: pitchPanelRoot.kitPrimary
                kitColorSecondary: pitchPanelRoot.kitSecondary
            }
        }
    }

    component MatchStatsPanel: Rectangle {
        radius: 12
        color: root.panelColor
        border.color: root.borderColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 18

            Label {
                text: "Match Statistics"
                color: root.textPrimary
                font.pixelSize: 20
                font.bold: true
            }

            StatComparisonRow { label: "Possession"; homeValue: "0%"; awayValue: "0%" }
            StatComparisonRow { label: "Expected Goals (xG)"; homeValue: "0.00"; awayValue: "0.00" }
            StatComparisonRow { label: "Total Shots"; homeValue: "0"; awayValue: "0" }
            StatComparisonRow { label: "Shots on Target"; homeValue: "0"; awayValue: "0" }
            StatComparisonRow { label: "Passes"; homeValue: "0"; awayValue: "0" }
            StatComparisonRow { label: "Pass Accuracy"; homeValue: "0%"; awayValue: "0%" }
            StatComparisonRow { label: "Fouls"; homeValue: "0"; awayValue: "0" }
            StatComparisonRow { label: "Corners"; homeValue: "0"; awayValue: "0" }

            Item { Layout.fillHeight: true }
        }
    }

    component StatComparisonRow: Item {
        property string label: ""
        property string homeValue: ""
        property string awayValue: ""

        Layout.fillWidth: true
        Layout.preferredHeight: 48

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            RowLayout {
                Layout.fillWidth: true

                Label { text: homeValue; color: root.textPrimary; font.pixelSize: 16; Layout.preferredWidth: 70 }
                Label { text: label; color: root.textPrimary; opacity: 0.92; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                Label { text: awayValue; color: root.textPrimary; font.pixelSize: 16; horizontalAlignment: Text.AlignRight; Layout.preferredWidth: 70 }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 3; color: "#1f7a4a" }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 3; color: "#7f2f25" }
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
        opacity: enabled ? 1.0 : 0.45
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
            enabled: buttonRoot.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: buttonRoot.clicked()
        }
    }
}

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
    property bool suppressFallback: false
    property var metrics: null
    readonly property bool compactLayout: metrics ? metrics.compact : width < 1400
    readonly property bool stackLineups: metrics ? metrics.narrow : width < 1180

    signal backRequested()
    signal editLineupRequested()
    signal playMatchRequested()

    readonly property color backgroundColor: "#071016"
    readonly property color shellColor: "#08111a"
    readonly property color panelColor: "#0f1a24"
    readonly property color borderColor: "#243442"
    readonly property color textPrimary: "#f5f7fa"
    readonly property color textSecondary: "#a8b3c1"
    readonly property color textMuted: "#64748b"
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
                Layout.preferredHeight: root.metrics ? root.metrics.px(root.metrics.dense ? 58 : 74) : 74
                color: root.shellColor
                border.color: root.borderColor

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: root.metrics ? root.metrics.pageMargin : 28
                    anchors.rightMargin: root.metrics ? root.metrics.pageMargin : 28
                    spacing: root.metrics ? root.metrics.spacingMd : 20

                    MatchTeamBadge {
                        Layout.preferredWidth: root.metrics ? root.metrics.px(44) : 44
                        Layout.preferredHeight: root.metrics ? root.metrics.px(44) : 44
                        badgeSize: 44
                        metrics: root.metrics
                        teamName: root.selectedTeamName || interactionData.homeTeamName || "No Team"
                        primaryColor: root.selectedTeamPrimaryColor
                        secondaryColor: root.selectedTeamSecondaryColor
                        textColor: root.selectedTeamTextColor
                    }

                    Label {
                        text: root.selectedTeamName || "No Team"
                        color: root.textPrimary
                        font.pixelSize: root.metrics ? root.metrics.font(18) : 18
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
                        font.pixelSize: root.metrics ? root.metrics.font(root.compactLayout ? 20 : 24) : 24
                        font.bold: true
                    }

                    Label {
                        text: root.currentDateText || interactionData.dateText || ""
                        color: root.textSecondary
                        font.pixelSize: root.metrics ? root.metrics.font(16) : 16
                    }

                    MatchFlowButton {
                        Layout.preferredWidth: root.metrics ? root.metrics.px(126) : 126
                        Layout.preferredHeight: root.metrics ? root.metrics.px(root.compactLayout ? 38 : 44) : 44
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
                sourceComponent: root.hasMatchData() ? matchContent : (root.suppressFallback ? emptyContent : fallbackContent)
            }
        }
    }

    Component {
        id: emptyContent

        Item {
            Rectangle {
                anchors.fill: parent
                color: root.backgroundColor
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
                anchors.leftMargin: root.metrics ? root.metrics.pageMargin : 30
                anchors.rightMargin: root.metrics ? root.metrics.pageMargin : 30
                anchors.topMargin: root.metrics ? root.metrics.spacingLg : 34
                anchors.bottomMargin: root.metrics ? root.metrics.spacingLg : 24
                spacing: root.metrics ? root.metrics.panelGap : 38

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.metrics ? root.metrics.px(root.compactLayout ? 96 : 132) : 132
                    spacing: root.metrics ? root.metrics.panelGap : 28

                    TeamHero {
                        Layout.fillWidth: true
                        teamName: interactionData.homeTeamName || "Home"
                        primaryColor: interactionData.homePrimaryColor || "#22c55e"
                        secondaryColor: interactionData.homeSecondaryColor || "#0f172a"
                        badgeTextColor: interactionData.homeTextColor || "#f8fafc"
                        recentForm: interactionData.homeRecentForm || ""
                        alignRight: true
                    }

                    Column {
                        Layout.preferredWidth: root.metrics ? root.metrics.px(root.compactLayout ? 210 : 260) : 260
                        spacing: root.metrics ? root.metrics.spacingSm : 14

                        Label {
                            width: parent.width
                            text: "VS"
                            color: root.textPrimary
                            font.pixelSize: root.metrics ? root.metrics.font(root.compactLayout ? 32 : 42) : 42
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 14
                            Label { text: interactionData.dateText || ""; color: root.textSecondary; font.pixelSize: root.metrics ? root.metrics.font(15) : 15 }
                            Rectangle { width: 5; height: 5; radius: 3; color: root.green; anchors.verticalCenter: parent.verticalCenter }
                            Label { text: "Matchweek " + (interactionData.matchweek || "-"); color: root.textSecondary; font.pixelSize: root.metrics ? root.metrics.font(15) : 15 }
                        }
                    }

                    TeamHero {
                        Layout.fillWidth: true
                        teamName: interactionData.awayTeamName || "Away"
                        primaryColor: interactionData.awayPrimaryColor || "#22c55e"
                        secondaryColor: interactionData.awaySecondaryColor || "#0f172a"
                        badgeTextColor: interactionData.awayTextColor || "#f8fafc"
                        recentForm: interactionData.awayRecentForm || ""
                        alignRight: false
                    }
                }

                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    sourceComponent: root.stackLineups ? stackedPitchPanels : sideBySidePitchPanels
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.metrics ? root.metrics.panelGap : 18

                    Item { Layout.fillWidth: true }

                    MatchFlowButton {
                        Layout.preferredWidth: root.metrics ? root.metrics.px(root.compactLayout ? 190 : 220) : 220
                        Layout.preferredHeight: root.metrics ? root.metrics.px(root.compactLayout ? 48 : 58) : 58
                        text: "Edit Lineup"
                        iconName: "lineup"
                        primary: false
                        onClicked: root.editLineupRequested()
                    }

                    MatchFlowButton {
                        Layout.preferredWidth: root.metrics ? root.metrics.px(root.compactLayout ? 220 : 260) : 260
                        Layout.preferredHeight: root.metrics ? root.metrics.px(root.compactLayout ? 48 : 58) : 58
                        text: "Play Match"
                        iconName: "play"
                        primary: true
                        onClicked: root.playMatchRequested()
                    }
                }
            }
        }
    }

    Component {
        id: sideBySidePitchPanels

        RowLayout {
            spacing: root.metrics ? root.metrics.panelGap : 28

            PitchPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                label: "HOME LINEUP"
                teamName: interactionData.homeTeamName || "Home"
                formationText: interactionData.homeFormationText || "-"
                lineupRows: interactionData.homeLineup || []
                averageOverallText: interactionData.homeAverageOverallText || "--"
                kitPrimary: interactionData.homePrimaryColor || "#22c55e"
                kitSecondary: interactionData.homeSecondaryColor || "#0f172a"
            }

            PitchPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                label: "AWAY LINEUP"
                teamName: interactionData.awayTeamName || "Away"
                formationText: interactionData.awayFormationText || "-"
                lineupRows: interactionData.awayLineup || []
                averageOverallText: interactionData.awayAverageOverallText || "--"
                kitPrimary: interactionData.awayPrimaryColor || "#22c55e"
                kitSecondary: interactionData.awaySecondaryColor || "#0f172a"
            }
        }
    }

    Component {
        id: stackedPitchPanels

        ColumnLayout {
            spacing: root.metrics ? root.metrics.panelGap : 28

            PitchPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                label: "HOME LINEUP"
                teamName: interactionData.homeTeamName || "Home"
                formationText: interactionData.homeFormationText || "-"
                lineupRows: interactionData.homeLineup || []
                averageOverallText: interactionData.homeAverageOverallText || "--"
                kitPrimary: interactionData.homePrimaryColor || "#22c55e"
                kitSecondary: interactionData.homeSecondaryColor || "#0f172a"
            }

            PitchPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                label: "AWAY LINEUP"
                teamName: interactionData.awayTeamName || "Away"
                formationText: interactionData.awayFormationText || "-"
                lineupRows: interactionData.awayLineup || []
                averageOverallText: interactionData.awayAverageOverallText || "--"
                kitPrimary: interactionData.awayPrimaryColor || "#22c55e"
                kitSecondary: interactionData.awaySecondaryColor || "#0f172a"
            }
        }
    }

    component TeamHero: Item {
        id: teamHeroRoot
        property string teamName: ""
        property string primaryColor: "#22c55e"
        property string secondaryColor: "#0f172a"
        property string badgeTextColor: "#f8fafc"
        property string recentForm: ""
        property bool alignRight: false

        RowLayout {
            anchors.fill: parent
            layoutDirection: alignRight ? Qt.RightToLeft : Qt.LeftToRight
            spacing: root.metrics ? root.metrics.panelGap : 22

            MatchTeamBadge {
                Layout.preferredWidth: root.metrics ? root.metrics.px(root.compactLayout ? 68 : 92) : 92
                Layout.preferredHeight: root.metrics ? root.metrics.px(root.compactLayout ? 68 : 92) : 92
                badgeSize: 92
                metrics: root.metrics
                teamName: teamHeroRoot.teamName
                primaryColor: teamHeroRoot.primaryColor
                secondaryColor: teamHeroRoot.secondaryColor
                textColor: teamHeroRoot.badgeTextColor
            }

            Column {
                Layout.fillWidth: true
                spacing: root.metrics ? root.metrics.spacingSm : 8

                Label {
                    width: parent.width
                    text: teamHeroRoot.teamName
                    color: root.textPrimary
                    font.pixelSize: root.metrics ? root.metrics.font(root.compactLayout ? 22 : 30) : 30
                    font.bold: true
                    horizontalAlignment: alignRight ? Text.AlignRight : Text.AlignLeft
                    elide: Text.ElideRight
                }

                RecentFormBoxes {
                    formText: teamHeroRoot.recentForm
                    alignRight: teamHeroRoot.alignRight
                    width: parent.width
                }
            }
        }
    }

    component RecentFormBoxes: Item {
        id: formRoot
        property string formText: ""
        property bool alignRight: false
        readonly property var results: formCharacters(formText)

        height: root.metrics ? root.metrics.px(24) : 24

        function formCharacters(value) {
            const cleaned = String(value || "").toUpperCase().replace(/[^WDL]/g, "")
            const chars = cleaned.length ? cleaned.split("").slice(0, 5) : []
            while (chars.length < 5) {
                chars.push("")
            }
            return chars
        }

        function resultColor(result) {
            if (result === "W") {
                return root.green
            }
            if (result === "D") {
                return "#d97706"
            }
            if (result === "L") {
                return "#dc2626"
            }
            return root.borderColor
        }

        function resultBackground(result) {
            if (result === "W") {
                return "#123824"
            }
            if (result === "D") {
                return "#35270f"
            }
            if (result === "L") {
                return "#351616"
            }
            return "#0b141d"
        }

        Row {
            anchors.right: formRoot.alignRight ? parent.right : undefined
            anchors.left: formRoot.alignRight ? undefined : parent.left
            anchors.verticalCenter: parent.verticalCenter
            spacing: root.metrics ? root.metrics.spacingXs : 6

            Repeater {
                model: formRoot.results

                Rectangle {
                    width: root.metrics ? root.metrics.px(24) : 24
                    height: root.metrics ? root.metrics.px(24) : 24
                    radius: root.metrics ? root.metrics.radiusSm : 5
                    color: formRoot.resultBackground(modelData)
                    border.color: formRoot.resultColor(modelData)
                    opacity: modelData.length > 0 ? 1.0 : 0.65

                    Label {
                        anchors.centerIn: parent
                        text: modelData
                        color: modelData.length > 0 ? root.textPrimary : root.textMuted
                        font.pixelSize: root.metrics ? root.metrics.font(11) : 11
                        font.bold: true
                    }
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
        property string averageOverallText: "--"
        property string kitPrimary: "#f97316"
        property string kitSecondary: "#22c55e"

        radius: root.metrics ? root.metrics.radiusLg : 12
        color: root.panelColor
        border.color: root.borderColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.metrics ? root.metrics.cardPadding : 20
            spacing: root.metrics ? root.metrics.spacingMd : 16

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: root.metrics ? root.metrics.px(root.compactLayout ? 50 : 62) : 62

                AppIcon {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    name: "lineup"
                    size: 26
                    opacity: 0.95
                    metrics: root.metrics
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 2
                    Label { text: label; color: root.green; font.pixelSize: root.metrics ? root.metrics.font(13) : 13; font.bold: true }
                    Label { text: teamName; color: root.textPrimary; font.pixelSize: root.metrics ? root.metrics.font(root.compactLayout ? 18 : 22) : 22; font.bold: true; elide: Text.ElideRight; width: parent.width }
                }

                Column {
                    spacing: 4
                    Label { text: "Formation: " + (formationText || "-"); color: root.textSecondary; font.pixelSize: root.metrics ? root.metrics.font(14) : 14; horizontalAlignment: Text.AlignRight; width: root.metrics ? root.metrics.px(root.compactLayout ? 126 : 160) : 160 }
                    Label { text: "Avg XI OVR " + (pitchPanelRoot.averageOverallText || "--"); color: root.textSecondary; font.pixelSize: root.metrics ? root.metrics.font(14) : 14; horizontalAlignment: Text.AlignRight; width: root.metrics ? root.metrics.px(root.compactLayout ? 126 : 160) : 160 }
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
                mode: "preMatch"
                kitColorPrimary: pitchPanelRoot.kitPrimary
                kitColorSecondary: pitchPanelRoot.kitSecondary
                metrics: root.metrics
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
                metrics: root.metrics
                anchors.verticalCenter: parent.verticalCenter
            }

            Label {
                text: buttonRoot.text
                color: root.textPrimary
                font.pixelSize: root.metrics ? root.metrics.font(17) : 17
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    signal openStandingsRequested()
    signal openTeamRequested()
    signal openLineupEditorRequested()
    signal openTransfersRequested()
    signal pauseRequested()
    signal resumeRequested()

    readonly property var dashboardState: gameFacade.dashboardState
    readonly property var shellState: gameFacade.shellState
    readonly property var nextMatch: dashboardState.nextMatch
    readonly property var teamStats: dashboardState.teamStats
    readonly property var standingsRow: dashboardState.standingsRow
    readonly property bool hasActiveGame: dashboardState.hasData
    readonly property bool timePaused: shellState.timePaused

    readonly property color backgroundColor: "#071016"
    readonly property color shellColor: "#08111a"
    readonly property color panelColor: "#0f1a24"
    readonly property color panelAltColor: "#101f2b"
    readonly property color borderColor: "#243442"
    readonly property color textPrimary: "#f5f7fa"
    readonly property color textSecondary: "#a8b3c1"
    readonly property color textMuted: "#64748b"
    readonly property color green: "#22c55e"
    readonly property color red: "#b91c1c"

    function teamName() {
        return dashboardState.selectedTeamName || "No Team"
    }

    function currentDateText() {
        return dashboardState.currentDateText || shellState.currentDateText || ""
    }

    function formCharacters() {
        const rawForm = String(dashboardState.recentForm || "")
        const cleaned = rawForm.toUpperCase().replace(/[^WDL]/g, "")
        const chars = cleaned.length ? cleaned.split("").slice(0, 5) : []
        while (chars.length < 5) {
            chars.push("")
        }
        return chars
    }

    function resultColor(result) {
        if (result === "W") {
            return green
        }
        if (result === "D") {
            return "#f59e0b"
        }
        if (result === "L") {
            return "#ef4444"
        }
        return borderColor
    }

    function resultBackground(result) {
        if (result === "W") {
            return "#123824"
        }
        if (result === "D") {
            return "#3a2c12"
        }
        if (result === "L") {
            return "#3a1717"
        }
        return "#0b141d"
    }

    function recordText() {
        return teamStats.wins + "W " + teamStats.draws + "D " + teamStats.losses + "L"
    }

    function pointsText() {
        if (standingsRow.hasData) {
            return standingsRow.points
        }
        return teamStats.points
    }

    function positionText() {
        return standingsRow.hasData ? standingsRow.position + getOrdinalSuffix(standingsRow.position) : "--"
    }

    function getOrdinalSuffix(position) {
        const mod100 = position % 100
        if (mod100 >= 11 && mod100 <= 13) {
            return "th"
        }
        switch (position % 10) {
        case 1:
            return "st"
        case 2:
            return "nd"
        case 3:
            return "rd"
        default:
            return "th"
        }
    }

    function matchTitle(homeTeam, awayTeam) {
        return (homeTeam || "") + " vs " + (awayTeam || "")
    }

    function matchLocation(isHome) {
        return isHome ? "Home" : "Away"
    }

    function matchweekText(matchweek) {
        return "Matchweek " + (matchweek || "--")
    }

    function dateParts(dateText) {
        const text = String(dateText || "")
        const pieces = text.replace(",", "").split(" ").filter(function(part) { return part.length > 0 })
        if (pieces.length >= 3) {
            return {
                month: pieces[0].slice(0, 3).toUpperCase(),
                day: pieces[1],
                year: pieces[2]
            }
        }
        return {
            month: "",
            day: text || "--",
            year: ""
        }
    }

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 18

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 90
                radius: 12
                color: root.shellColor
                border.color: root.borderColor

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 24
                    anchors.rightMargin: 16
                    spacing: 18

                    Rectangle {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        radius: 24
                        color: "#f8fafc"
                        border.color: root.green

                        Rectangle {
                            anchors.centerIn: parent
                            width: 26
                            height: 34
                            radius: 8
                            color: "#f97316"
                            border.color: "#0f172a"
                        }

                        Label {
                            anchors.centerIn: parent
                            text: "A"
                            font.pixelSize: 20
                            font.bold: true
                            color: "#0f172a"
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.teamName()
                        color: root.textPrimary
                        font.pixelSize: 28
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        text: root.currentDateText()
                        color: root.textPrimary
                        opacity: 0.9
                        font.pixelSize: 18
                    }

                    Rectangle {
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: 60
                        color: root.borderColor
                    }

                    ShellActionButton {
                        Layout.preferredWidth: 190
                        Layout.preferredHeight: 62
                        text: root.timePaused ? "Resume" : "Pause"
                        accentColor: root.timePaused ? root.green : root.red
                        glyph: root.timePaused ? ">" : "||"
                        enabled: root.hasActiveGame
                        onClicked: {
                            if (root.timePaused) {
                                root.resumeRequested()
                            } else {
                                root.pauseRequested()
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 20

                Rectangle {
                    Layout.preferredWidth: 190
                    Layout.fillHeight: true
                    radius: 12
                    color: root.shellColor
                    border.color: root.borderColor
                    visible: root.hasActiveGame

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 10

                        SidebarItem {
                            Layout.fillWidth: true
                            label: "Standings"
                            glyph: "T"
                            onClicked: root.openStandingsRequested()
                        }

                        SidebarItem {
                            Layout.fillWidth: true
                            label: "Team"
                            glyph: "P"
                            onClicked: root.openTeamRequested()
                        }

                        SidebarItem {
                            Layout.fillWidth: true
                            label: "Lineup Editor"
                            glyph: "XI"
                            onClicked: root.openLineupEditorRequested()
                        }

                        SidebarItem {
                            Layout.fillWidth: true
                            label: "Transfers"
                            glyph: "$"
                            onClicked: root.openTransfersRequested()
                        }

                        SidebarItem {
                            Layout.fillWidth: true
                            label: "Fixtures"
                            glyph: "C"
                            enabled: false
                        }

                        Item {
                            Layout.fillHeight: true
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: root.borderColor
                        }

                        SidebarItem {
                            Layout.fillWidth: true
                            label: "Settings"
                            glyph: "*"
                            enabled: false
                        }

                        SidebarItem {
                            Layout.fillWidth: true
                            label: "Main Menu"
                            glyph: "<"
                            enabled: false
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Rectangle {
                        anchors.fill: parent
                        radius: 12
                        color: root.panelColor
                        border.color: root.borderColor
                        visible: !root.hasActiveGame

                        Column {
                            anchors.centerIn: parent
                            width: Math.min(parent.width - 64, 560)
                            spacing: 12

                            Label {
                                width: parent.width
                                text: "Start a new game from the home screen to view the dashboard."
                                color: root.textPrimary
                                font.pixelSize: 24
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                            }

                            Label {
                                width: parent.width
                                text: "Once a club is selected, this dark dashboard will show the next match, season stats and upcoming schedule."
                                color: root.textSecondary
                                font.pixelSize: 16
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    ScrollView {
                        id: contentScroll
                        anchors.fill: parent
                        clip: true
                        visible: root.hasActiveGame
                        contentWidth: availableWidth

                        GridLayout {
                            width: contentScroll.availableWidth
                            columns: width >= 980 ? 2 : 1
                            columnSpacing: 18
                            rowSpacing: 18

                            DashboardCard {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 424
                                title: "NEXT MATCH"
                                titleColor: root.green

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 30
                                    anchors.topMargin: 72
                                    spacing: 18

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 162

                                        RowLayout {
                                            anchors.fill: parent
                                            spacing: 24

                                            TeamMark {
                                                Layout.fillWidth: true
                                                teamName: root.nextMatch && root.nextMatch.hasNextMatch
                                                          ? root.nextMatch.homeTeamName
                                                          : root.teamName()
                                            }

                                            Label {
                                                text: "VS"
                                                color: root.textPrimary
                                                opacity: 0.82
                                                font.pixelSize: 25
                                                font.bold: true
                                            }

                                            TeamMark {
                                                Layout.fillWidth: true
                                                teamName: root.nextMatch && root.nextMatch.hasNextMatch
                                                          ? root.nextMatch.awayTeamName
                                                          : "Opponent"
                                                alt: true
                                            }
                                        }
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: root.nextMatch && root.nextMatch.hasNextMatch
                                              ? root.nextMatch.dateText || "--"
                                              : "No upcoming match"
                                        color: root.textPrimary
                                        opacity: 0.92
                                        font.pixelSize: 20
                                        horizontalAlignment: Text.AlignHCenter
                                        elide: Text.ElideRight
                                    }

                                    RowLayout {
                                        Layout.alignment: Qt.AlignHCenter
                                        spacing: 12
                                        visible: root.nextMatch && root.nextMatch.hasNextMatch

                                        Label {
                                            text: root.matchLocation(root.nextMatch.isHome)
                                            color: root.textPrimary
                                            font.pixelSize: 17
                                        }

                                        Rectangle {
                                            Layout.preferredWidth: 5
                                            Layout.preferredHeight: 5
                                            radius: 3
                                            color: root.green
                                        }

                                        Label {
                                            text: root.matchweekText(root.nextMatch.matchweek)
                                            color: root.textPrimary
                                            font.pixelSize: 17
                                        }
                                    }

                                    Rectangle {
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.preferredWidth: 270
                                        Layout.preferredHeight: 48
                                        radius: 12
                                        color: "#0c1720"
                                        border.color: root.borderColor

                                        Label {
                                            anchors.centerIn: parent
                                            text: root.nextMatch && root.nextMatch.hasNextMatch ? "Upcoming match" : "Schedule pending"
                                            color: root.textSecondary
                                            font.pixelSize: 17
                                        }
                                    }
                                }
                            }

                            DashboardCard {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 424
                                title: "Season Stats"
                                glyph: "|||"

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    anchors.topMargin: 74
                                    spacing: 0

                                    StatRow {
                                        Layout.fillWidth: true
                                        glyph: "T"
                                        label: "League Position"
                                        value: root.positionText()
                                        valueColor: root.green
                                    }

                                    StatRow {
                                        Layout.fillWidth: true
                                        glyph: "*"
                                        label: "Points"
                                        value: String(root.pointsText())
                                    }

                                    StatRow {
                                        Layout.fillWidth: true
                                        glyph: "R"
                                        label: "Record"
                                        value: root.recordText()
                                    }

                                    StatRow {
                                        Layout.fillWidth: true
                                        glyph: "G"
                                        label: "Goals For / Against"
                                        value: root.teamStats.goalsFor + " / " + root.teamStats.goalsAgainst
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 62
                                        color: "transparent"

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 12
                                            anchors.rightMargin: 8
                                            spacing: 16

                                            Label {
                                                Layout.preferredWidth: 28
                                                text: "~"
                                                color: root.textSecondary
                                                font.pixelSize: 24
                                                horizontalAlignment: Text.AlignHCenter
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: "Recent Form"
                                                color: root.textSecondary
                                                font.pixelSize: 18
                                                elide: Text.ElideRight
                                            }

                                            Row {
                                                spacing: 10

                                                Repeater {
                                                    model: root.formCharacters()

                                                    delegate: Rectangle {
                                                        required property string modelData
                                                        width: 32
                                                        height: 32
                                                        radius: 5
                                                        color: root.resultBackground(modelData)
                                                        border.color: root.resultColor(modelData)

                                                        Label {
                                                            anchors.centerIn: parent
                                                            text: modelData
                                                            color: root.resultColor(modelData)
                                                            font.pixelSize: 13
                                                            font.bold: true
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        Rectangle {
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.bottom: parent.bottom
                                            height: 1
                                            color: root.borderColor
                                            opacity: 0.7
                                        }
                                    }
                                }
                            }

                            DashboardCard {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 444
                                title: "Schedule Overview"
                                glyph: "C"
                                headerRightText: "View Full Schedule"
                                headerRightEnabled: false

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 20
                                    anchors.topMargin: 72
                                    spacing: 0

                                    Repeater {
                                        id: upcomingMatchesRepeater
                                        model: gameFacade.dashboardUpcomingMatchesModel

                                        delegate: FixtureRow {
                                            required property int index

                                            Layout.fillWidth: true
                                            visible: index < 3
                                            dateText: model.dateText || ""
                                            title: root.matchTitle(model.homeTeamName, model.awayTeamName)
                                            locationText: root.matchLocation(model.isHome)
                                            matchweekText: root.matchweekText(model.matchweek)
                                            homeFixture: model.isHome
                                        }
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        visible: upcomingMatchesRepeater.count === 0
                                        text: "No upcoming match"
                                        color: root.textSecondary
                                        font.pixelSize: 16
                                        horizontalAlignment: Text.AlignHCenter
                                        topPadding: 60
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 444
                            }
                        }
                    }
                }
            }
        }
    }

    component ShellActionButton: Item {
        id: actionRoot
        signal clicked()
        property string text: ""
        property string glyph: ""
        property color accentColor: root.green
        property bool enabled: true

        opacity: enabled ? 1.0 : 0.45

        Rectangle {
            anchors.fill: parent
            radius: 10
            color: !actionRoot.enabled
                   ? "#14202a"
                   : actionMouse.pressed
                     ? Qt.darker(actionRoot.accentColor, 1.25)
                     : actionMouse.containsMouse
                       ? Qt.lighter(actionRoot.accentColor, 1.08)
                       : actionRoot.accentColor
            border.color: Qt.lighter(actionRoot.accentColor, 1.1)

            Row {
                anchors.centerIn: parent
                spacing: 12

                Label {
                    text: actionRoot.glyph
                    color: "#ffffff"
                    font.pixelSize: 21
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }

                Label {
                    text: actionRoot.text
                    color: "#ffffff"
                    font.pixelSize: 20
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                id: actionMouse
                anchors.fill: parent
                hoverEnabled: true
                enabled: actionRoot.enabled
                cursorShape: Qt.PointingHandCursor
                onClicked: actionRoot.clicked()
            }
        }
    }

    component SidebarItem: Item {
        id: navRoot
        signal clicked()
        property string label: ""
        property string glyph: ""
        property bool enabled: true

        Layout.preferredHeight: 58
        opacity: enabled ? 1.0 : 0.42

        Rectangle {
            anchors.fill: parent
            radius: 8
            color: navRoot.enabled && (navMouse.containsMouse || navMouse.pressed) ? "#10291d" : "transparent"
            border.color: navRoot.enabled && navMouse.containsMouse ? root.green : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 14

                Label {
                    Layout.preferredWidth: 30
                    text: navRoot.glyph
                    color: navRoot.enabled && navMouse.containsMouse ? root.green : root.textSecondary
                    font.pixelSize: 18
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    Layout.fillWidth: true
                    text: navRoot.label
                    color: root.textPrimary
                    opacity: navRoot.enabled ? 0.92 : 0.7
                    font.pixelSize: 18
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                id: navMouse
                anchors.fill: parent
                hoverEnabled: true
                enabled: navRoot.enabled
                cursorShape: Qt.PointingHandCursor
                onClicked: navRoot.clicked()
            }
        }
    }

    component DashboardCard: Rectangle {
        id: cardRoot
        property string title: ""
        property string glyph: ""
        property string headerRightText: ""
        property bool headerRightEnabled: true
        property color titleColor: root.textPrimary

        radius: 12
        color: root.panelColor
        border.color: root.borderColor
        clip: true

        Rectangle {
            anchors.fill: parent
            color: "transparent"

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#0b1922" }
                GradientStop { position: 1.0; color: root.panelColor }
            }
            opacity: 0.72
        }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 30
            anchors.rightMargin: 24
            anchors.topMargin: 28
            spacing: 14

            Label {
                visible: cardRoot.glyph.length > 0
                text: cardRoot.glyph
                color: root.green
                font.pixelSize: 24
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: cardRoot.title
                color: cardRoot.titleColor
                font.pixelSize: cardRoot.title === cardRoot.title.toUpperCase() ? 16 : 23
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                visible: cardRoot.headerRightText.length > 0
                text: cardRoot.headerRightText
                color: root.green
                opacity: cardRoot.headerRightEnabled ? 1.0 : 0.55
                font.pixelSize: 15
            }

            Label {
                visible: cardRoot.headerRightText.length > 0
                text: ">"
                color: root.textSecondary
                font.pixelSize: 24
            }
        }
    }

    component TeamMark: Item {
        id: teamMarkRoot
        property string teamName: ""
        property bool alt: false

        Layout.preferredHeight: 160

        Column {
            anchors.centerIn: parent
            width: parent.width
            spacing: 14

            Rectangle {
                width: 100
                height: 100
                radius: 50
                color: "#f8fafc"
                border.color: teamMarkRoot.alt ? "#38bdf8" : root.green
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    anchors.centerIn: parent
                    width: 48
                    height: 62
                    radius: 11
                    color: teamMarkRoot.alt ? "#1d4ed8" : "#f97316"
                    border.color: "#0f172a"
                }

                Label {
                    anchors.centerIn: parent
                    text: teamMarkRoot.teamName.length > 0 ? teamMarkRoot.teamName.charAt(0).toUpperCase() : "?"
                    color: "#0f172a"
                    font.pixelSize: 30
                    font.bold: true
                }
            }

            Label {
                width: parent.width
                text: teamMarkRoot.teamName || "--"
                color: root.textPrimary
                font.pixelSize: 23
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
        }
    }

    component StatRow: Rectangle {
        id: statRoot
        property string glyph: ""
        property string label: ""
        property string value: ""
        property color valueColor: root.textPrimary

        height: 62
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 8
            spacing: 16

            Label {
                Layout.preferredWidth: 28
                text: statRoot.glyph
                color: root.textSecondary
                font.pixelSize: 21
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                Layout.fillWidth: true
                text: statRoot.label
                color: root.textSecondary
                font.pixelSize: 18
                elide: Text.ElideRight
            }

            Label {
                text: statRoot.value
                color: statRoot.valueColor
                font.pixelSize: 18
                horizontalAlignment: Text.AlignRight
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: root.borderColor
            opacity: 0.7
        }
    }

    component FixtureRow: Rectangle {
        id: fixtureRoot
        property string dateText: ""
        property string title: ""
        property string locationText: ""
        property string matchweekText: ""
        property bool homeFixture: false
        readonly property var parts: root.dateParts(dateText)

        Layout.preferredHeight: 102
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            spacing: 16

            Column {
                Layout.preferredWidth: 58
                spacing: 2

                Label {
                    width: parent.width
                    text: fixtureRoot.parts.month
                    color: root.green
                    font.pixelSize: 14
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    width: parent.width
                    text: fixtureRoot.parts.day
                    color: root.textPrimary
                    font.pixelSize: 26
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    width: parent.width
                    text: fixtureRoot.parts.year
                    color: root.textSecondary
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: root.borderColor
                opacity: 0.75
            }

            Rectangle {
                Layout.preferredWidth: 58
                Layout.preferredHeight: 58
                radius: 29
                color: root.panelAltColor
                border.color: root.borderColor

                Label {
                    anchors.centerIn: parent
                    text: fixtureRoot.title.length > 0 ? fixtureRoot.title.charAt(0).toUpperCase() : "?"
                    color: root.textPrimary
                    font.pixelSize: 22
                    font.bold: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 7

                Label {
                    Layout.fillWidth: true
                    text: fixtureRoot.title
                    color: root.textPrimary
                    font.pixelSize: 18
                    font.bold: true
                    elide: Text.ElideRight
                }

                RowLayout {
                    spacing: 10

                    Label {
                        text: fixtureRoot.locationText
                        color: root.textSecondary
                        font.pixelSize: 16
                    }

                    Rectangle {
                        Layout.preferredWidth: 5
                        Layout.preferredHeight: 5
                        radius: 3
                        color: root.green
                    }

                    Label {
                        text: fixtureRoot.matchweekText
                        color: root.textSecondary
                        font.pixelSize: 16
                    }
                }
            }

            Label {
                text: fixtureRoot.homeFixture ? "H" : "A"
                color: fixtureRoot.homeFixture ? root.green : root.textSecondary
                font.pixelSize: 22
                font.bold: true
            }

            Label {
                text: ">"
                color: root.textSecondary
                font.pixelSize: 28
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: root.borderColor
            opacity: 0.65
        }
    }
}

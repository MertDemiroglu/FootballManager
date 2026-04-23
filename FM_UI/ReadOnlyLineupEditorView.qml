import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var gameFacade
    property var lineupSummary: ({})
    property var slotRows: []
    property var rosterRows: []

    implicitHeight: panelsLayout.implicitHeight

    function refreshData() {
        if (!gameFacade) {
            lineupSummary = ({})
            slotRows = []
            rosterRows = []
            return
        }

        lineupSummary = gameFacade.getEditableLineupSummary()
        slotRows = gameFacade.getEditableLineupSlots()
        rosterRows = gameFacade.getEditableLineupRoster()
    }

    function conditionColor(value) {
        const numeric = Number(value)
        if (numeric >= 75)
            return "#16a34a"
        if (numeric >= 50)
            return "#eab308"
        if (numeric >= 25)
            return "#f97316"
        return "#dc2626"
    }

    function roleGroup(slotRow) {
        const role = slotRow.slotRole || ""
        if (role === "GK")
            return 0
        if (role === "LB" || role === "CB" || role === "RB" || role === "LWB" || role === "RWB")
            return 1
        if (role === "DM" || role === "CM" || role === "AM" || role === "LM" || role === "RM")
            return 2
        return 3
    }

    function slotsForGroup(groupIndex) {
        const grouped = []
        for (let i = 0; i < slotRows.length; ++i) {
            const row = slotRows[i]
            if (roleGroup(row) === groupIndex)
                grouped.push(row)
        }
        return grouped
    }

    Component.onCompleted: refreshData()

    Connections {
        target: gameFacade
        function onGameStateChanged() {
            root.refreshData()
        }
    }

    ColumnLayout {
        id: panelsLayout
        width: parent ? parent.width : 0
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: "Read-Only Lineup Editor"
            font.pixelSize: 24
            font.bold: true
            color: "#17212f"
        }

        Label {
            Layout.fillWidth: true
            text: lineupSummary.hasLineup
                  ? "Formation " + (lineupSummary.formationText || "-") + " • Assigned " + (lineupSummary.assignedCount || 0) + "/" + (lineupSummary.slotCount || 0)
                  : "Lineup data unavailable"
            font.pixelSize: 14
            color: "#526071"
            wrapMode: Text.WordWrap
        }

        Loader {
            Layout.fillWidth: true
            sourceComponent: wideLayout
        }
    }

    Component {
        id: wideLayout

        RowLayout {
            width: parent ? parent.width : 0
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                radius: 14
                border.color: "#d8dee8"
                color: "#ffffff"
                implicitHeight: 520

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10

                    Label {
                        text: "Pitch"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#17212f"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 12
                        color: "#1e7a46"
                        border.color: "#0f5132"

                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width * 0.88
                            height: parent.height * 0.88
                            color: "transparent"
                            border.width: 2
                            border.color: "#ffffff"
                            radius: 10
                        }

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width * 0.88
                            height: 2
                            color: "#ffffff"
                        }

                        Column {
                            anchors.fill: parent
                            anchors.margins: 22
                            spacing: 10

                            Repeater {
                                model: 4
                                delegate: Row {
                                    required property int index
                                    width: parent.width
                                    height: (parent.height - 30) / 4
                                    spacing: 8

                                    Repeater {
                                        model: root.slotsForGroup(index)
                                        delegate: Rectangle {
                                            required property var modelData
                                            width: Math.max(90, (parent.width - ((parent.children.length - 1) * parent.spacing)) / Math.max(1, parent.children.length))
                                            height: parent.height - 6
                                            radius: 8
                                            color: modelData.isEmpty ? "#fef3c7" : "#f8fafc"
                                            border.color: modelData.isEmpty ? "#f59e0b" : "#d0d5dd"

                                            Column {
                                                anchors.fill: parent
                                                anchors.margins: 6
                                                spacing: 2

                                                Label {
                                                    width: parent.width
                                                    text: modelData.slotLabel || modelData.slotRole || "Slot"
                                                    font.pixelSize: 11
                                                    font.bold: true
                                                    color: "#344054"
                                                    horizontalAlignment: Text.AlignHCenter
                                                    elide: Text.ElideRight
                                                }

                                                Label {
                                                    width: parent.width
                                                    text: modelData.isEmpty ? "Empty" : (modelData.assignedPlayerName || "Unknown")
                                                    font.pixelSize: 11
                                                    font.bold: !modelData.isEmpty
                                                    color: modelData.isEmpty ? "#92400e" : "#101828"
                                                    horizontalAlignment: Text.AlignHCenter
                                                    elide: Text.ElideRight
                                                }

                                                Label {
                                                    width: parent.width
                                                    text: modelData.isEmpty ? "—" : (modelData.assignedPlayerOverallSummary || "OVR -")
                                                    font.pixelSize: 10
                                                    color: "#475467"
                                                    horizontalAlignment: Text.AlignHCenter
                                                    elide: Text.ElideRight
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                radius: 14
                border.color: "#d8dee8"
                color: "#ffffff"
                implicitHeight: 520

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10

                    Label {
                        text: "Roster"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#17212f"
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ListView {
                            width: parent.width
                            model: root.rosterRows
                            spacing: 8
                            delegate: rosterDelegate
                        }
                    }
                }
            }
        }
    }

    Component {
        id: rosterDelegate

        Rectangle {
            required property var modelData
            width: ListView.view ? ListView.view.width : 0
            radius: 10
            border.color: modelData.isAssigned ? "#93c5fd" : "#e4e7ec"
            color: modelData.isAssigned ? "#eff6ff" : "#f8fafc"
            opacity: modelData.isAssigned ? 0.8 : 1.0
            implicitHeight: rosterContent.implicitHeight + 18

            ColumnLayout {
                id: rosterContent
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        radius: 6
                        color: "#eef2ff"
                        border.color: "#c7d2fe"
                        implicitWidth: 38
                        implicitHeight: 24

                        Label {
                            anchors.centerIn: parent
                            text: modelData.positionShort || "?"
                            font.bold: true
                            font.pixelSize: 11
                            color: "#3730a3"
                        }
                    }

                    Label {
                        text: modelData.overallSummary || ("OVR " + (modelData.overall || "-"))
                        font.pixelSize: 12
                        color: "#475467"
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData.name || "Unknown"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#101828"
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        visible: modelData.isAssigned
                        radius: 999
                        color: "#dbeafe"
                        border.color: "#60a5fa"
                        implicitWidth: 78
                        implicitHeight: 24

                        Label {
                            anchors.centerIn: parent
                            text: modelData.isAssigned ? "Assigned" : ""
                            font.pixelSize: 11
                            font.bold: true
                            color: "#1d4ed8"
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: [
                            { label: "Form", value: modelData.form },
                            { label: "Fit", value: modelData.fitness },
                            { label: "Mor", value: modelData.morale }
                        ]

                        delegate: Rectangle {
                            required property var modelData
                            Layout.preferredWidth: 86
                            Layout.preferredHeight: 24
                            radius: 999
                            color: Qt.rgba(0, 0, 0, 0.08)
                            border.color: root.conditionColor(modelData.value)

                            Label {
                                anchors.centerIn: parent
                                text: modelData.label + " " + modelData.value
                                font.pixelSize: 11
                                font.bold: true
                                color: root.conditionColor(modelData.value)
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        text: modelData.isAssigned ? ("Slot #" + modelData.assignedSlotIndex) : "Unassigned"
                        font.pixelSize: 11
                        color: modelData.isAssigned ? "#1d4ed8" : "#667085"
                    }
                }
            }
        }
    }
}

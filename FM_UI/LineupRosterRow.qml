import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var rowData: ({})

    radius: 10
    border.color: rowData.isAssigned ? "#93c5fd" : "#e4e7ec"
    color: rowData.isAssigned ? "#eff6ff" : "#f8fafc"
    opacity: rowData.isAssigned ? 0.8 : 1.0
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
                    text: rowData.positionShort || "?"
                    font.bold: true
                    font.pixelSize: 11
                    color: "#3730a3"
                }
            }

            Label {
                text: rowData.overallSummary || ("OVR " + (rowData.overall || "-"))
                font.pixelSize: 12
                color: "#475467"
            }

            Label {
                Layout.fillWidth: true
                text: rowData.name || "Unknown"
                font.pixelSize: 14
                font.bold: true
                color: "#101828"
                elide: Text.ElideRight
            }

            Rectangle {
                visible: rowData.isAssigned
                radius: 999
                color: "#dbeafe"
                border.color: "#60a5fa"
                implicitWidth: 78
                implicitHeight: 24

                Label {
                    anchors.centerIn: parent
                    text: rowData.isAssigned ? "Assigned" : ""
                    font.pixelSize: 11
                    font.bold: true
                    color: "#1d4ed8"
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ConditionBadge {
                label: "Form"
                value: rowData.form
            }

            ConditionBadge {
                label: "Fit"
                value: rowData.fitness
            }

            ConditionBadge {
                label: "Mor"
                value: rowData.morale
            }

            Item { Layout.fillWidth: true }

            Label {
                text: rowData.isAssigned ? ("Slot #" + rowData.assignedSlotIndex) : "Unassigned"
                font.pixelSize: 11
                color: rowData.isAssigned ? "#1d4ed8" : "#667085"
            }
        }
    }
}

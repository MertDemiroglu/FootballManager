import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string positionText: ""
    property string playerName: ""
    property string surname: ""
    property string displayName: ""
    property string kitColorPrimary: "#f97316"
    property string kitColorSecondary: "#22c55e"
    property string positionGroup: ""
    property string metricText: ""
    property bool showMetric: false
    property string mode: "preMatch"
    property bool empty: false

    readonly property string resolvedName: root.empty
                                          ? "Empty"
                                          : (root.surname.length > 0
                                             ? root.surname
                                             : surnameFromName(root.displayName.length > 0 ? root.displayName : root.playerName))
    readonly property string resolvedGroup: root.positionGroup.length > 0
                                            ? root.positionGroup
                                            : groupFromPosition(root.positionText)

    width: 82
    height: root.showMetric ? 72 : 62

    function surnameFromName(fullName) {
        const parts = String(fullName || "").trim().split(/\s+/).filter(function(part) { return part.length > 0 })
        return parts.length > 0 ? parts[parts.length - 1] : "Empty"
    }

    function groupFromPosition(position) {
        const text = String(position || "").toUpperCase()
        if (text === "GK") {
            return "GK"
        }
        if (text.indexOf("B") >= 0 || text === "DF") {
            return "DEF"
        }
        if (text.indexOf("M") >= 0) {
            return "MID"
        }
        if (text.indexOf("ST") >= 0 || text.indexOf("F") >= 0 || text.indexOf("W") >= 0) {
            return "FWD"
        }
        return "UNK"
    }

    function bandColor(group) {
        switch (group) {
        case "GK":
            return "#16a34a"
        case "DEF":
            return "#2563eb"
        case "MID":
            return "#f59e0b"
        case "FWD":
            return "#dc2626"
        default:
            return "#64748b"
        }
    }

    Item {
        id: shirt
        width: 32
        height: 25
        anchors.horizontalCenter: parent.horizontalCenter
        y: 0
        visible: !root.empty

        Rectangle {
            x: 6
            y: 5
            width: 20
            height: 18
            radius: 5
            color: root.kitColorPrimary
            border.color: "#0f172a"
        }

        Rectangle {
            x: 0
            y: 7
            width: 10
            height: 11
            radius: 4
            rotation: -18
            color: root.kitColorSecondary
            border.color: "#0f172a"
        }

        Rectangle {
            x: 22
            y: 7
            width: 10
            height: 11
            radius: 4
            rotation: 18
            color: root.kitColorSecondary
            border.color: "#0f172a"
        }

        Rectangle {
            x: 14
            y: 5
            width: 4
            height: 18
            radius: 2
            color: root.kitColorSecondary
            opacity: 0.9
        }
    }

    Rectangle {
        id: body
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.empty ? 14 : 23
        width: 72
        height: root.showMetric ? 44 : 36
        radius: 7
        color: root.empty ? "#111827" : "#111923"
        border.color: root.empty ? "#334155" : "#405264"
        border.width: 1

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 18
            radius: 6
            color: root.empty ? "#334155" : root.bandColor(root.resolvedGroup)

            Label {
                anchors.centerIn: parent
                text: root.positionText || "-"
                color: "#f8fafc"
                font.pixelSize: 11
                font.bold: true
            }
        }

        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 20
            text: root.resolvedName
            color: root.empty ? "#94a3b8" : "#f8fafc"
            font.pixelSize: 13
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }

        Rectangle {
            width: 36
            height: 17
            radius: 8
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 3
            visible: root.showMetric
            color: "#0b1118"
            border.color: "#405264"

            Label {
                anchors.centerIn: parent
                text: root.metricText.length > 0 ? root.metricText : "-"
                color: root.metricText.length > 0 ? "#f8fafc" : "#64748b"
                font.pixelSize: 11
                font.bold: true
            }
        }
    }
}

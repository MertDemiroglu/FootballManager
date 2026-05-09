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

    width: 86
    height: root.showMetric ? 82 : 70

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
            return "#15803d"
        case "DEF":
            return "#1d4ed8"
        case "MID":
            return "#d97706"
        case "FWD":
            return "#b91c1c"
        default:
            return "#475569"
        }
    }

    Item {
        id: shirt
        width: 40
        height: 31
        anchors.horizontalCenter: parent.horizontalCenter
        y: 0
        visible: !root.empty

        Rectangle {
            x: 8
            y: 6
            width: 24
            height: 22
            radius: 6
            color: root.kitColorPrimary
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            x: 0
            y: 8
            width: 13
            height: 13
            radius: 4
            rotation: -18
            color: root.kitColorSecondary
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            x: 27
            y: 8
            width: 13
            height: 13
            radius: 4
            rotation: 18
            color: root.kitColorSecondary
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            x: 18
            y: 6
            width: 4
            height: 22
            radius: 2
            color: root.kitColorSecondary
            opacity: 0.9
        }
    }

    Rectangle {
        id: body
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.empty ? 19 : 27
        width: 82
        height: root.showMetric ? 52 : 40
        radius: 7
        color: root.empty ? "#111827" : "#111923"
        border.color: root.empty ? "#334155" : "#405264"
        border.width: 1

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 19
            radius: 6
            color: root.empty ? "#334155" : root.bandColor(root.resolvedGroup)

            Label {
                anchors.centerIn: parent
                text: root.positionText || "-"
                color: "#f8fafc"
                font.pixelSize: 12
                font.bold: true
            }
        }

        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 21
            height: 20
            text: root.resolvedName
            color: root.empty ? "#94a3b8" : "#f8fafc"
            font.pixelSize: 13
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }

        Rectangle {
            width: 40
            height: 17
            radius: 8
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
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

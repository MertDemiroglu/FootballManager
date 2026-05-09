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

    width: 90
    height: root.showMetric ? 90 : 76

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
        width: 46
        height: 36
        anchors.horizontalCenter: parent.horizontalCenter
        y: 0
        visible: !root.empty

        Rectangle {
            x: 10
            y: 7
            width: 26
            height: 25
            radius: 7
            color: root.kitColorPrimary
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            x: 0
            y: 10
            width: 16
            height: 14
            radius: 4
            rotation: -18
            color: root.kitColorSecondary
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            x: 30
            y: 10
            width: 16
            height: 14
            radius: 4
            rotation: 18
            color: root.kitColorSecondary
            border.color: "#0f172a"
            border.width: 1
        }

        Rectangle {
            x: 21
            y: 7
            width: 5
            height: 25
            radius: 2
            color: root.kitColorSecondary
            opacity: 0.9
        }

        Rectangle {
            x: 17
            y: 6
            width: 12
            height: 7
            radius: 4
            color: "#dbeafe"
            opacity: 0.9
            border.color: "#0f172a"
            border.width: 1
        }
    }

    Rectangle {
        id: body
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.empty ? 21 : 30
        width: 86
        height: root.showMetric ? 58 : 44
        radius: 7
        color: root.empty ? "#111827" : "#111923"
        border.color: root.empty ? "#334155" : "#405264"
        border.width: 1

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 20
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
            anchors.topMargin: 23
            height: 23
            text: root.resolvedName
            color: root.empty ? "#94a3b8" : "#f8fafc"
            font.pixelSize: 14
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }

        Rectangle {
            width: 42
            height: 18
            radius: 8
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 5
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

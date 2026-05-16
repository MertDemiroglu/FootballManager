import QtQuick

Item {
    id: root

    property string name: ""
    property int size: 24
    property color color: "#a8b3c1"

    implicitWidth: size
    implicitHeight: size

    function iconFile(iconName) {
        switch (iconName) {
        case "standings":
            return "standings.svg"
        case "team":
            return "team.svg"
        case "lineup":
            return "lineup.svg"
        case "transfers":
            return "transfers.svg"
        case "fixtures":
            return "fixtures.svg"
        case "calendar":
            return "calendar.svg"
        case "settings":
            return "settings.svg"
        case "save":
            return "save.svg"
        case "main-menu":
            return "main-menu.svg"
        case "trophy":
            return "trophy.svg"
        case "star":
            return "star.svg"
        case "record":
            return "record.svg"
        case "ball":
            return "ball.svg"
        case "form":
            return "form.svg"
        case "play":
            return "play.svg"
        case "pause":
            return "pause.svg"
        case "home":
            return "home.svg"
        case "away":
            return "away.svg"
        case "stats":
            return "stats.svg"
        case "chevron-right":
            return "chevron-right.svg"
        default:
            return ""
        }
    }

    Image {
        anchors.centerIn: parent
        width: root.size
        height: root.size
        source: root.iconFile(root.name).length > 0
                ? Qt.resolvedUrl("assets/icons/" + root.iconFile(root.name))
                : ""
        sourceSize.width: root.size
        sourceSize.height: root.size
        fillMode: Image.PreserveAspectFit
    }
}

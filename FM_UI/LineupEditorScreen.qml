import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    anchors.fill: parent
    color: "#071018"

    readonly property var shellState: gameFacade ? gameFacade.shellState : null

    signal backRequested()

    function prepareLineup() {
        if (gameFacade) {
            gameFacade.ensureEditableLineupReady()
        }
    }

    Component.onCompleted: prepareLineup()
    onVisibleChanged: {
        if (visible) {
            prepareLineup()
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 28
        anchors.rightMargin: 28
        anchors.topMargin: 18
        anchors.bottomMargin: 24
        spacing: 14

        Item {
            Layout.fillWidth: true
            implicitHeight: 44

            RowLayout {
                id: headerContent
                anchors.fill: parent
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    radius: 8
                    color: "#10251a"
                    border.color: "#2fb565"

                    Label {
                        anchors.centerIn: parent
                        text: "XI"
                        font.pixelSize: 13
                        font.bold: true
                        color: "#7ee2a8"
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: shellState && shellState.selectedTeamName.length > 0
                          ? shellState.selectedTeamName
                          : (gameFacade ? gameFacade.getSelectedTeamName() : "Team")
                    font.pixelSize: 18
                    font.bold: true
                    color: "#f7fbff"
                    elide: Text.ElideRight
                }

                Label {
                    visible: shellState && shellState.currentDateText.length > 0
                    text: shellState ? shellState.currentDateText : ""
                    font.pixelSize: 14
                    color: "#d6dde5"
                }

                Button {
                    text: "Back"
                    Layout.preferredWidth: 108
                    Layout.preferredHeight: 34
                    onClicked: root.backRequested()
                    contentItem: Label {
                        text: parent.text
                        color: "#d8e2ec"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 8
                        color: parent.down ? "#1d2b38" : "#121e29"
                        border.color: parent.hovered ? "#496174" : "#2c3d4c"
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: "#1b2833"
            opacity: 0.9
        }

        ScrollView {
            id: editorScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            contentHeight: editorContent.height
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical: ScrollBar {
                id: editorVerticalScroll
                policy: ScrollBar.AsNeeded
                width: 8
                contentItem: Rectangle {
                    implicitWidth: 8
                    radius: 4
                    color: editorVerticalScroll.pressed || editorVerticalScroll.hovered ? "#53697a" : "#2c3d4c"
                }
                background: Rectangle {
                    radius: 4
                    color: "#071018"
                }
            }

            LineupEditorView {
                id: editorContent
                width: editorScroll.availableWidth
                height: Math.max(editorScroll.availableHeight, implicitHeight)
            }
        }
    }
}

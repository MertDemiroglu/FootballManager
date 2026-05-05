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
        anchors.margins: 20
        spacing: 16

        Rectangle {
            Layout.fillWidth: true
            radius: 14
            color: "#0f1a24"
            border.color: "#243443"
            border.width: 1
            implicitHeight: headerContent.implicitHeight + 28

            RowLayout {
                id: headerContent
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.topMargin: 14
                anchors.bottomMargin: 14
                spacing: 14

                Rectangle {
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 44
                    radius: 10
                    color: "#13251d"
                    border.color: "#1f7a46"

                    Label {
                        anchors.centerIn: parent
                        text: "XI"
                        font.pixelSize: 15
                        font.bold: true
                        color: "#7ee2a8"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: shellState && shellState.selectedTeamName.length > 0
                              ? shellState.selectedTeamName
                              : (gameFacade ? gameFacade.getSelectedTeamName() : "Team")
                        font.pixelSize: 13
                        font.bold: true
                        color: "#8fa3b8"
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Lineup Editor"
                        font.pixelSize: 30
                        font.bold: true
                        color: "#f7fbff"
                        elide: Text.ElideRight
                    }
                }

                Label {
                    visible: shellState && shellState.currentDateText.length > 0
                    text: shellState ? shellState.currentDateText : ""
                    font.pixelSize: 13
                    color: "#9aaebe"
                }

                Button {
                    text: "Back"
                    Layout.preferredWidth: 108
                    Layout.preferredHeight: 40
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

        ScrollView {
            id: editorScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            contentHeight: editorContent.height

            LineupEditorView {
                id: editorContent
                width: editorScroll.availableWidth
                height: Math.max(editorScroll.availableHeight, implicitHeight)
            }
        }
    }
}

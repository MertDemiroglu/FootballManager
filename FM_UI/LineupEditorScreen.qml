import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    anchors.fill: parent
    color: "#071018"

    readonly property var shellState: gameFacade ? gameFacade.shellState : null
    property var metrics: null

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
        anchors.leftMargin: metrics ? metrics.pageMargin : 28
        anchors.rightMargin: metrics ? metrics.pageMargin : 28
        anchors.topMargin: metrics ? metrics.spacingLg : 18
        anchors.bottomMargin: metrics ? metrics.spacingLg : 24
        spacing: metrics ? metrics.spacingMd : 14

        Item {
            Layout.fillWidth: true
            implicitHeight: metrics ? metrics.px(metrics.dense ? 36 : 44) : 44

            RowLayout {
                id: headerContent
                anchors.fill: parent
                spacing: metrics ? metrics.spacingMd : 12

                Rectangle {
                    Layout.preferredWidth: metrics ? metrics.px(34) : 34
                    Layout.preferredHeight: metrics ? metrics.px(34) : 34
                    radius: metrics ? metrics.radiusMd : 8
                    color: "#10251a"
                    border.color: "#2fb565"

                    Label {
                        anchors.centerIn: parent
                        text: "XI"
                        font.pixelSize: metrics ? metrics.font(13) : 13
                        font.bold: true
                        color: "#7ee2a8"
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: shellState && shellState.selectedTeamName.length > 0
                          ? shellState.selectedTeamName
                          : (gameFacade ? gameFacade.getSelectedTeamName() : "Team")
                    font.pixelSize: metrics ? metrics.font(18) : 18
                    font.bold: true
                    color: "#f7fbff"
                    elide: Text.ElideRight
                }

                Label {
                    visible: shellState && shellState.currentDateText.length > 0
                    text: shellState ? shellState.currentDateText : ""
                    font.pixelSize: metrics ? metrics.font(14) : 14
                    color: "#d6dde5"
                }

                Button {
                    text: "Back"
                    Layout.preferredWidth: metrics ? metrics.px(108) : 108
                    Layout.preferredHeight: metrics ? metrics.px(34) : 34
                    onClicked: root.backRequested()
                    contentItem: Label {
                        text: parent.text
                        color: "#d8e2ec"
                        font.pixelSize: metrics ? metrics.font(13) : 13
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

        LineupEditorView {
            id: editorContent
            Layout.fillWidth: true
            Layout.fillHeight: true
            metrics: root.metrics
        }
    }
}

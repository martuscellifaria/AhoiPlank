import QtQuick
import QtQuick.Layouts

Window {
    id: window
    width: theme.screen_width
    height: theme.screen_height
    flags: Qt.FramelessWindowHint
    color: "transparent"
    title: "launcher"
    visible: false

    property bool was_active: false

    onActiveChanged: {
        if (active) {
            was_active = true
        } else if (was_active && visible) {
            was_active = false
            app.dismiss()
        }
    }

    Shortcut { sequence: "Esc";    onActivated: app.dismiss() }
    Shortcut { sequence: "Return"; onActivated: app.launch(list.currentIndex) }

    Component.onCompleted: search.forceActiveFocus()

    Rectangle {
        anchors.fill: parent
        radius: theme.radius_window
        color: theme.bg
        border.color: theme.border
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Rectangle {
                id: search_bg
                Layout.fillWidth: true
                implicitHeight: search.implicitHeight + 16
                radius: theme.radius_item
                color: theme.bg_input
                border.color: theme.border
                border.width: 1

                TextInput {
                    id: search
                    anchors.fill: parent
                    anchors.margins: 8
                    verticalAlignment: TextInput.AlignVCenter
                    color: theme.fg_dim
                    font.family: theme.font_family
                    font.pixelSize: theme.font_size_search
                    clip: true
                    selectByMouse: true
                    onTextChanged: app.set_filter(text)

                    Text {
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        text: "Search…"
                        color: theme.fg_dim
                        opacity: 0.5
                        font: search.font
                        visible: search.text.length === 0
                    }
                    Keys.onDownPressed: (event) => {
                        event.accepted = true
                        if (list.count === 0) return
                        list.incrementCurrentIndex()
                        list.positionViewAtIndex(list.currentIndex, ListView.Contain)
                    }
                    Keys.onUpPressed: (event) => {
                        event.accepted = true
                        if (list.count === 0) return
                        list.decrementCurrentIndex()
                        list.positionViewAtIndex(list.currentIndex, ListView.Contain)
                    }
                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_PageDown) {
                            event.accepted = true
                            if (list.count === 0) return
                            list.currentIndex = Math.min(list.currentIndex + 5, list.count - 1)
                            list.positionViewAtIndex(list.currentIndex, ListView.Contain)
                        } else if (event.key === Qt.Key_PageUp) {
                            event.accepted = true
                            if (list.count === 0) return
                            list.currentIndex = Math.max(list.currentIndex - 5, 0)
                            list.positionViewAtIndex(list.currentIndex, ListView.Contain)
                        }
                    }
                }
            }

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: app
                currentIndex: count > 0 ? 0 : -1
                delegate: Rectangle {
                    required property string name
                    required property string exec
                    required property string comment
                    required property string icon

                    width: list.width
                    height: 44
                    radius: theme.radius_item
                    color: ListView.isCurrentItem ? theme.selection : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10

                        Item {
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            Layout.alignment: Qt.AlignVCenter

                            Image {
                                anchors.centerIn: parent
                                source: icon.length > 0 ? "image://icon/" + icon : ""
                                sourceSize: Qt.size(24, 24)
                                width: 24
                                height: 24
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                visible: status === Image.Ready
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 0

                            Text {
                                Layout.fillWidth: true
                                text: parent.parent.parent.name
                                color: theme.fg
                                font.family: theme.font_family
                                font.pixelSize: theme.font_size_title
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: parent.parent.parent.comment
                                color: theme.fg_dim
                                font.family: theme.font_family
                                font.pixelSize: theme.font_size_comment
                                elide: Text.ElideRight
                                visible: text.length > 0
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: app.launch(index)
                    }
                }
            }
        }
    }
}

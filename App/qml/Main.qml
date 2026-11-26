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

    readonly property int columns: Math.max(1, theme.columns)
    readonly property bool bar_on_top: theme.status_bar_position !== "bottom"
    readonly property var mode_order: ["standard", "bookmarks", "dotfiles", "ssh", "projects", "scripts"]
    readonly property int mode_slot_count: Math.min(10, mode_order.length)

    onActiveChanged: {
        if (active) {
            was_active = true
            search.forceActiveFocus()
        }
        else if (was_active && visible) {
            was_active = false
            app.dismiss()
        }
    }

    Connections {
        target: app
        function onResetRequested() {
            search.text = ""
            grid.currentIndex = grid.count > 0 ? 0 : -1
            search.forceActiveFocus()
        }
        function onModeChanged() {
            grid.currentIndex = grid.count > 0 ? 0 : -1
            search.forceActiveFocus()
            if (search.text !== "") {
                app.set_filter(search.text)
            }
        }
    }

    Shortcut { sequence: "Esc";    onActivated: app.dismiss() }
    Shortcut { sequence: "Return"; onActivated: app.launch(grid.currentIndex, search.text) }

    Rectangle {
        anchors.fill: parent
        radius: theme.radius_window
        color: theme.bg
        border.color: theme.border
        border.width: theme.border_width

        Item {
            id: layout
            anchors.fill: parent
            anchors.margins: theme.layout_margin

            StatusBar {
                id: status_bar
                visible: theme.status_bar_visible
                barStyle: theme.status_bar_style || "tmux"
                mode: app.modeString
                modes: window.mode_order
                background: theme.status_bar_bg
                foreground: theme.status_bar_fg
                accent: theme.status_bar_pill
                accentText: theme.bg
                barHeight: theme.status_bar_height
                radius: theme.radius_item
                pillPadding: theme.status_bar_pill_padding
                fontFamily: theme.font_family
                fontSize: theme.font_size_comment
                fontBold: theme.font_bold
                hintText: isVimStyle ? "[Alt+←/→]" : "[Alt+index]"
                x: 0
                width: layout.width
                y: window.bar_on_top ? 0 : layout.height - height
            }

            ColumnLayout {
                spacing: theme.layout_spacing

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: window.bar_on_top && status_bar.visible
                             ? status_bar.bottom
                             : parent.top
                anchors.bottom: !window.bar_on_top && status_bar.visible
                                ? status_bar.top
                                : parent.bottom
                anchors.topMargin: window.bar_on_top && status_bar.visible
                                   ? theme.status_bar_gap : 0
                anchors.bottomMargin: !window.bar_on_top && status_bar.visible
                                      ? theme.status_bar_gap : 0

                Rectangle {
                    id: search_bg
                    Layout.fillWidth: true
                    implicitHeight: search.implicitHeight + 2 * theme.search_padding
                    radius: theme.radius_item
                    color: theme.bg_input
                    border.color: theme.border
                    border.width: theme.border_width

                    TextInput {
                        id: search
                        anchors.fill: parent
                        anchors.margins: theme.search_padding
                        verticalAlignment: TextInput.AlignVCenter
                        color: theme.fg_dim
                        font.family: theme.font_family
                        font.pixelSize: theme.font_size_search
                        font.bold: theme.font_bold
                        clip: true
                        selectByMouse: true
                        onTextChanged: {
                            app.set_filter(text)
                            grid.currentIndex = grid.count > 0 ? 0 : -1
                            grid.positionViewAtBeginning()
                        }

                        Text {
                            anchors.fill: parent
                            verticalAlignment: Text.AlignVCenter
                            text: {
                                const _ = app.modeString
                                return app.mode_placeholder()
                            }
                            color: theme.fg_dim
                            opacity: 0.5
                            font: search.font
                            visible: search.text.length === 0
                        }

                        Keys.onLeftPressed: (event) => {
                            event.accepted = true
                            if (event.modifiers & Qt.ControlModifier) {
                                search.cursorPosition = Math.max(0, search.cursorPosition - 1)
                                return
                            }
                            if (status_bar.isVimStyle && (event.modifiers & Qt.AltModifier)) {
                                app.cycle_mode(-1)
                                return
                            }
                            if (grid.count === 0) return
                            grid.currentIndex = Math.max(0, grid.currentIndex - 1)
                            grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        }
                        Keys.onRightPressed: (event) => {
                            event.accepted = true
                            if (event.modifiers & Qt.ControlModifier) {
                                search.cursorPosition = Math.min(search.length, search.cursorPosition + 1)
                                return
                            }
                            if (status_bar.isVimStyle && (event.modifiers & Qt.AltModifier)) {
                                app.cycle_mode(1)
                                return
                            }
                            if (grid.count === 0) return
                            grid.currentIndex = Math.min(grid.count - 1, grid.currentIndex + 1)
                            grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        }
                        Keys.onDownPressed: (event) => {
                            event.accepted = true
                            if (grid.count === 0) return
                            const next = grid.currentIndex + columns
                            grid.currentIndex = Math.min(next, grid.count - 1)
                            grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        }
                        Keys.onUpPressed: (event) => {
                            event.accepted = true
                            if (grid.count === 0) return
                            const prev = grid.currentIndex - columns
                            grid.currentIndex = Math.max(prev, 0)
                            grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        }
                        Keys.onPressed: (event) => {
                            if (!status_bar.isVimStyle && (event.modifiers & Qt.AltModifier)) {
                                let slot = -1
                                if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9)
                                    slot = event.key - Qt.Key_1
                                else if (event.key === Qt.Key_0)
                                    slot = 9
                                else if (event.text.length === 1) {
                                    const c = event.text.charCodeAt(0)
                                    if (c >= 0x31 && c <= 0x39)
                                        slot = c - 0x31
                                    else if (c === 0x30)
                                        slot = 9
                                }
                                if (slot >= 0 && slot < window.mode_slot_count) {
                                    event.accepted = true
                                    app.set_mode(window.mode_order[slot])
                                }
                            }
                        }
                    }
                }

                GridView {
                    id: grid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: app
                    currentIndex: count > 0 ? 0 : -1

                    cellWidth:  Math.floor(width / columns)
                    cellHeight: theme.item_height

                    highlightFollowsCurrentItem: true
                    highlightMoveDuration: 100

                    delegate: Rectangle {
                        required property string name
                        required property string exec
                        required property string comment
                        required property string icon

                        width: grid.cellWidth
                        height: grid.cellHeight
                        radius: theme.radius_item
                        color: GridView.isCurrentItem ? theme.selection : "transparent"

                        Image {
                            id: row_icon
                            anchors.left: parent.left
                            anchors.leftMargin: theme.item_padding
                            anchors.verticalCenter: parent.verticalCenter
                            source: icon.length > 0 ? "image://icon/" + icon : ""
                            sourceSize: Qt.size(theme.icon_size, theme.icon_size)
                            width: theme.icon_size
                            height: theme.icon_size
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            visible: status === Image.Ready
                        }

                        Text {
                            anchors.left: row_icon.right
                            anchors.leftMargin: theme.icon_spacing
                            anchors.right: parent.right
                            anchors.rightMargin: theme.item_padding
                            anchors.top: parent.top
                            anchors.topMargin: 6
                            text: name
                            color: theme.fg
                            font.family: theme.font_family
                            font.pixelSize: theme.font_size_title
                            font.bold: theme.font_bold
                            elide: Text.ElideRight
                        }

                        Text {
                            anchors.left: row_icon.right
                            anchors.leftMargin: theme.icon_spacing
                            anchors.right: parent.right
                            anchors.rightMargin: theme.item_padding
                            anchors.top: parent.top
                            anchors.topMargin: 6 + theme.font_size_title + 2
                            text: comment
                            color: theme.fg_dim
                            font.family: theme.font_family
                            font.pixelSize: theme.font_size_comment
                            font.bold: theme.font_bold
                            elide: Text.ElideRight
                            visible: text.length > 0
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: app.launch(index, search.text)
                        }
                    }
                }
            }
        }
    }
}

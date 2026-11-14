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
    readonly property var mode_order: ["standard", "bookmark", "dotfiles", "ssh"]
    readonly property int current_mode_index: {
        const i = mode_order.indexOf(app.modeString)
        return i < 0 ? 0 : i
    }
    readonly property string next_mode_label:
        mode_order[(current_mode_index + 1) % mode_order.length].toUpperCase()
    readonly property string prev_mode_label:
        mode_order[(current_mode_index - 1 + mode_order.length) % mode_order.length].toUpperCase()

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
            search.text = ""
            grid.currentIndex = grid.count > 0 ? 0 : -1
            search.forceActiveFocus()
        }
    }

    Shortcut { sequence: "Esc";    onActivated: app.dismiss() }
    Shortcut { sequence: "Return"; onActivated: app.launch(grid.currentIndex) }

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
                mode: app.modeString
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
                nextModeLabel: window.next_mode_label
                prevModeLabel: window.prev_mode_label

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: window.bar_on_top ? parent.top : undefined
                anchors.bottom: window.bar_on_top ? undefined : parent.bottom
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
                            if (grid.count === 0) return
                            grid.currentIndex = Math.max(0, grid.currentIndex - 1)
                            grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                        }
                        Keys.onRightPressed: (event) => {
                            event.accepted = true
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
                            if (event.key === Qt.Key_Tab) {
                                event.accepted = true
                                app.cycle_mode(event.modifiers & Qt.ShiftModifier ? -1 : 1)
                                return
                            }
                            if (event.key === Qt.Key_Backtab) {
                                event.accepted = true
                                app.cycle_mode(-1)
                                return
                            }
                            if (event.key === Qt.Key_PageDown) {
                                event.accepted = true
                                if (grid.count === 0) return
                                const step = 5 * columns
                                grid.currentIndex = Math.min(grid.currentIndex + step, grid.count - 1)
                                grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                            } else if (event.key === Qt.Key_PageUp) {
                                event.accepted = true
                                if (grid.count === 0) return
                                const step = 5 * columns
                                grid.currentIndex = Math.max(grid.currentIndex - step, 0)
                                grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
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
                            onClicked: app.launch(index)
                        }
                    }
                }
            }
        }
    }
}

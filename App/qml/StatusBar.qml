import QtQuick

Item {
    id: root

    property string barStyle: "tmux"
    property string mode: "standard"
    property var modes: ["standard", "bookmarks", "dotfiles", "ssh", "projects", "scripts"]

    property color background: "transparent"
    property color foreground: "#cdd6f4"
    property color accent: "#89b4fa"
    property color accentText: "#1e1e2e"
    property int barHeight: 22
    property int radius: 3
    property int pillPadding: 14
    property int itemSpacing: 4

    property string fontFamily: "monospace"
    property int fontSize: 12
    property bool fontBold: true

    property string hintText: ""

    property string message: ""
    property color messageBackground: "#f38ba8"
    property color messageForeground: "#1e1e2e"
    property int messageDuration: 4000
    property int messagePaddingH: 10
    property int messagePaddingV: 4

    readonly property bool hasMessage: message.length > 0

    implicitHeight: hasMessage ? Math.max(barHeight, messageText.implicitHeight + 2 * messagePaddingV) : barHeight
    implicitWidth: 0

    Behavior on implicitHeight {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutCubic
        }
    }

    function showMessage(msg) {
        if (!msg || msg.length === 0)
            return;
        root.message = msg;
        messageTimer.restart();
    }

    function dismissMessage() {
        messageTimer.stop();
        root.message = "";
    }

    Timer {
        id: messageTimer
        interval: root.messageDuration
        onTriggered: root.message = ""
    }

    function getModeLabel(m) {
        switch (m) {
        case "bookmarks":
            return "BOOKMARKS";
        case "dotfiles":
            return "DOTFILES";
        case "ssh":
            return "SSH";
        case "projects":
            return "PROJECTS";
        case "scripts":
            return "SCRIPTS";
        case "standard":
            return "NORMAL";
        default:
            return m ? m.toUpperCase() : "NORMAL";
        }
    }

    readonly property bool isVimStyle: root.barStyle.toLowerCase() === "vim"
    readonly property int activeIndex: Math.max(0, modes.indexOf(root.mode))

    function ensureActiveVisible() {
        if (root.isVimStyle || modeRepeater.count === 0)
            return;
        const item = modeRepeater.itemAt(root.activeIndex);
        if (!item)
            return;
        const itemLeft = item.x;
        const itemRight = item.x + item.width;
        const viewLeft = modeFlickable.contentX;
        const viewRight = modeFlickable.contentX + modeFlickable.width;
        if (itemLeft < viewLeft)
            modeFlickable.contentX = itemLeft;
        else if (itemRight > viewRight)
            modeFlickable.contentX = itemRight - modeFlickable.width;
    }

    onActiveIndexChanged: Qt.callLater(ensureActiveVisible)

    Rectangle {
        id: outer
        anchors.fill: parent
        color: root.background
        clip: true

        Item {
            id: normalBand
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: root.barHeight
            visible: !root.hasMessage
            clip: true

            Flickable {
                id: modeFlickable
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: hint.visible ? hint.left : parent.right
                anchors.rightMargin: 10
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                clip: true
                flickableDirection: Flickable.HorizontalFlick
                boundsBehavior: Flickable.StopAtBounds
                interactive: !root.isVimStyle && contentWidth > width
                contentWidth: root.isVimStyle ? vimBadge.width : modeRow.implicitWidth
                contentHeight: height

                onWidthChanged: root.ensureActiveVisible()
                onContentWidthChanged: root.ensureActiveVisible()

                Behavior on contentX {
                    NumberAnimation {
                        duration: 150
                        easing.type: Easing.OutCubic
                    }
                }

                WheelHandler {
                    enabled: modeFlickable.interactive
                    onWheel: event => {
                        const delta = event.angleDelta.x !== 0 ? event.angleDelta.x : event.angleDelta.y;
                        const maxScroll = Math.max(0, modeFlickable.contentWidth - modeFlickable.width);
                        modeFlickable.contentX = Math.max(0, Math.min(maxScroll, modeFlickable.contentX - delta * 0.5));
                        event.accepted = true;
                    }
                }

                Rectangle {
                    id: pill
                    anchors.verticalCenter: parent.verticalCenter
                    height: root.barHeight
                    radius: root.radius
                    color: root.accent
                    visible: root.isVimStyle ? vimBadge.visible : modeRepeater.count > 0

                    readonly property Item activeDelegate: modeRepeater.count > 0 ? modeRepeater.itemAt(root.activeIndex) : null

                    x: root.isVimStyle ? vimBadge.x : (activeDelegate ? activeDelegate.x : 0)
                    width: root.isVimStyle ? vimBadge.width : (activeDelegate ? activeDelegate.width : 0)

                    property bool animate: false
                    onWidthChanged: if (width > 0)
                        animate = true

                    Behavior on x {
                        enabled: pill.animate && !root.isVimStyle
                        NumberAnimation {
                            duration: 150
                            easing.type: Easing.OutCubic
                        }
                    }
                    Behavior on width {
                        enabled: pill.animate
                        NumberAnimation {
                            duration: 150
                            easing.type: Easing.OutCubic
                        }
                    }
                }

                Item {
                    id: vimBadge
                    visible: root.isVimStyle
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: vimText.implicitWidth + 2 * root.pillPadding
                    height: vimText.implicitHeight + 6

                    Text {
                        id: vimText
                        anchors.centerIn: parent
                        text: root.getModeLabel(root.mode)
                        color: root.accentText
                        font.family: root.fontFamily
                        font.pixelSize: root.fontSize
                        font.bold: root.fontBold
                    }
                }

                Row {
                    id: modeRow
                    visible: !root.isVimStyle
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: root.itemSpacing

                    Repeater {
                        id: modeRepeater
                        model: root.modes

                        delegate: Item {
                            id: modeDelegate
                            readonly property bool isActive: index === root.activeIndex

                            width: itemText.implicitWidth + 2 * root.pillPadding
                            height: itemText.implicitHeight + 6

                            Text {
                                id: itemText
                                anchors.centerIn: parent
                                text: `${index + 1}.${root.getModeLabel(modelData)}`
                                color: modeDelegate.isActive ? root.accentText : root.foreground
                                opacity: modeDelegate.isActive ? 1.0 : 0.65
                                font.family: root.fontFamily
                                font.pixelSize: root.fontSize
                                font.bold: root.fontBold
                                Behavior on color {
                                    ColorAnimation {
                                        duration: 150
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Text {
                id: hint
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, parent.width * 0.35)
                horizontalAlignment: Text.AlignRight
                text: root.hintText
                visible: root.hintText.length > 0
                color: root.foreground
                opacity: 0.55
                font.family: root.fontFamily
                font.pixelSize: root.fontSize
                font.bold: root.fontBold
                elide: Text.ElideLeft
            }
        }

        Rectangle {
            id: messageBand
            anchors.fill: parent
            color: root.messageBackground
            radius: root.radius
            visible: root.hasMessage
            z: 10

            Text {
                id: messageText
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 2 * root.messagePaddingH
                text: root.message
                color: root.messageForeground
                font.family: root.fontFamily
                font.pixelSize: root.fontSize
                font.bold: root.fontBold
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            TapHandler {
                onTapped: root.dismissMessage()
            }
            HoverHandler {
                cursorShape: Qt.PointingHandCursor
            }
        }
    }
}

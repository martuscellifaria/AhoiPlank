import QtQuick

Item {
    id: root

    property string message: ""
    property color background: "#f38ba8"
    property color foreground: "#1e1e2e"
    property color borderColor: "#eba0ac"
    property int borderWidth: 1
    property int radius: 6
    property int duration: 4000

    property string fontFamily: "monospace"
    property int fontSize: 12
    property bool fontBold: true

    readonly property bool active: hideTimer.running

    implicitWidth: Math.min(parent ? parent.width - 32 : 400, toastText.implicitWidth + 32)
    implicitHeight: toastText.implicitHeight + 16
    visible: opacity > 0
    opacity: 0.0

    function show(msg) {
        if (!msg || msg.length === 0) return
        root.message = msg
        hideTimer.restart()
        root.opacity = 1.0
    }

    function dismiss() {
        hideTimer.stop()
        root.opacity = 0.0
    }

    Behavior on opacity {
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }

    Timer {
        id: hideTimer
        interval: root.duration
        onTriggered: root.opacity = 0.0
    }

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.background
        border.color: root.borderColor
        border.width: root.borderWidth

        Text {
            id: toastText
            anchors.centerIn: parent
            width: parent.width - 24
            text: root.message
            color: root.foreground
            font.family: root.fontFamily
            font.pixelSize: root.fontSize
            font.bold: root.fontBold
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        TapHandler {
            onTapped: root.dismiss()
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }
    }
}

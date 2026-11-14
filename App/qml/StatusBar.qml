import QtQuick

Item {
    id: root

    property string mode: "standard"
    property color background: "transparent"
    property color foreground: "#cdd6f4"
    property color accent: "#89b4fa"
    property color accentText: "#1e1e2e"
    property int barHeight: 22
    property int radius: 3
    property int pillPadding: 14

    property string fontFamily: "monospace"
    property int fontSize: 12
    property bool fontBold: true

    property string nextModeLabel: "STANDARD"
    property string prevModeLabel: "SSH"

    implicitHeight: barHeight
    implicitWidth: 0

    readonly property string modeLabel: {
        switch (root.mode) {
            case "bookmark": return "BOOKMARK"
            case "dotfiles": return "DOTFILES"
            case "ssh":      return "SSH"
            case "standard": return "NORMAL"
            default:         return root.mode ? root.mode.toUpperCase() : "NORMAL"
        }
    }

    Rectangle {
        anchors.fill: parent
        color: root.background

        Rectangle {
            id: pill
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            width: label.implicitWidth + 2 * root.pillPadding
            height: label.implicitHeight + 6
            radius: root.radius
            color: root.accent

            Text {
                id: label
                anchors.centerIn: parent
                text: root.modeLabel
                color: root.accentText
                font.family: root.fontFamily
                font.pixelSize: root.fontSize
                font.bold: root.fontBold
            }
        }

        Text {
            id: hint
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: "<Tab> " + root.nextModeLabel + "   <Backtab> " + root.prevModeLabel
            color: root.foreground
            opacity: 0.55
            font.family: root.fontFamily
            font.pixelSize: root.fontSize
            font.bold: root.fontBold
            elide: Text.ElideLeft
        }
    }
}

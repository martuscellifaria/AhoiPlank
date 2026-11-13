import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string mode: "standard"
    property color background: "#1e1e2e"
    property color foreground: "#cdd6f4"
    property color accent: "#89b4fa"
    property int barHeight: 22

    implicitHeight: barHeight
    color: root.background

    readonly property string modeLabel: {
        switch (root.mode) {
            case "standard":  return "NORMAL"
            case "bookmark":  return "BOOKMARK"
            case "dotfiles":  return "DOTFILES"
            case "ssh":       return "SSH"
            default:          return root.mode.toUpperCase()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 12

        Rectangle {
            implicitWidth: modeText.implicitWidth + 14
            implicitHeight: root.barHeight - 6
            radius: 3
            color: root.accent

            Text {
                id: modeText
                anchors.centerIn: parent
                text: root.modeLabel
                color: root.background
                font.family: "monospace"
                font.bold: true
                font.pixelSize: 12
            }
        }

        Item { Layout.fillWidth: true }

        Text {
            text: root.mode
            color: root.foreground
            font.family: "monospace"
            font.pixelSize: 12
            opacity: 0.6
        }
    }
}

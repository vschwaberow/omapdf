import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: theme.darkBackground
    implicitHeight: theme.fontBaseSize + theme.spaceLg * 2 + theme.spaceSm * 2
    Accessible.name: qsTr("Search")

    property int hitCount: 0
    property int hitIndex: 0

    signal searchChanged(string text)
    signal forward()
    signal back()

    readonly property bool fieldFocus: field.activeFocus

    function forceActiveFocus() { field.forceActiveFocus() }
    function setText(text) {
        if (field.text === text)
            return
        field.text = text
    }
    function text() { return field.text }

    RowLayout {
        anchors.fill: parent
        anchors.margins: theme.spaceSm
        spacing: theme.spaceSm

        TextField {
            id: field
            Layout.fillWidth: true
            Accessible.name: qsTr("Search")
            placeholderText: qsTr("Search…")
            color: theme.foreground
            font.pixelSize: theme.fontBaseSize
            background: Rectangle {
                radius: theme.cornerRadius
                color: theme.withAlpha(theme.hoverColor,
                    field.activeFocus ? theme.focusFillAlpha : theme.normalFillAlpha)
                border.width: theme.controlBorderWidth
                border.color: field.activeFocus
                    ? theme.withAlpha(theme.focusBorder, theme.focusBorderAlpha)
                    : theme.withAlpha(theme.normalBorder, theme.normalBorderAlpha)
            }
            onTextChanged: searchDebounce.restart()
            Keys.onReturnPressed: {
                searchDebounce.stop()
                root.searchChanged(field.text)
                root.forward()
            }
            Keys.onEscapePressed: {
                searchDebounce.stop()
                if (text.length)
                    text = ""
                root.searchChanged(text)
                focus = false
            }
        }
        Text {
            visible: field.text.length > 0
            text: root.hitCount > 0
                ? (root.hitIndex + "/" + root.hitCount)
                : "0"
            color: theme.darkForeground
            font.pixelSize: theme.fontBaseSize - 1
            Accessible.name: qsTr("Search result")
            Layout.alignment: Qt.AlignVCenter
        }
        ThemedToolButton {
            text: "↑"
            Accessible.name: qsTr("Previous result")
            onClicked: root.back()
        }
        ThemedToolButton {
            text: "↓"
            Accessible.name: qsTr("Next result")
            onClicked: root.forward()
        }
    }

    Timer {
        id: searchDebounce
        interval: 40
        onTriggered: root.searchChanged(field.text)
    }
}

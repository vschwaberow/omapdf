import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    default property alias content: body.data
    property alias body: body
    property string accessibleName: ""

    signal escapePressed()

    modal: true
    anchors.centerIn: Overlay.overlay
    width: 360
    padding: theme.spaceLg

    background: Rectangle {
        color: theme.darkBackground
        border.color: theme.withAlpha(theme.focusBorder, theme.focusBorderAlpha)
        border.width: theme.controlBorderWidth
        radius: theme.cornerRadius
    }

    ColumnLayout {
        id: body
        anchors.fill: parent
        spacing: theme.spaceMd
        focus: true
        Accessible.name: root.accessibleName
        Keys.onEscapePressed: root.escapePressed()
    }
}

import QtQuick
import QtQuick.Controls

ToolButton {
    id: root

    property string glyph: ""
    property string hint: ""
    property bool active: false

    text: root.glyph
    checkable: false
    font.pixelSize: theme.fontBaseSize
    implicitWidth: theme.fontBaseSize + theme.spaceLg * 2
    implicitHeight: theme.fontBaseSize + theme.spaceLg * 2
    padding: theme.spaceSm
    Accessible.name: root.hint.length ? root.hint : root.glyph

    ToolTip.visible: root.hovered && root.hint.length > 0
    ToolTip.delay: 400
    ToolTip.text: root.hint

    background: Rectangle {
        radius: theme.cornerRadius
        color: {
            if (root.down)
                return theme.withAlpha(theme.hoverColor, theme.pressedFillAlpha)
            if (root.active)
                return theme.withAlpha(theme.selectedColor, theme.selectedFillAlpha)
            if (root.visualFocus)
                return theme.withAlpha(theme.focusColor, theme.focusFillAlpha)
            if (root.hovered)
                return theme.withAlpha(theme.hoverColor, theme.hoverFillAlpha)
            return "transparent"
        }
        border.width: root.visualFocus || root.hovered || root.active
                        ? theme.controlBorderWidth : 0
        border.color: {
            if (root.active)
                return theme.withAlpha(theme.selectedBorder, theme.selectedBorderAlpha)
            if (root.visualFocus)
                return theme.withAlpha(theme.focusBorder, theme.focusBorderAlpha)
            return theme.withAlpha(theme.hoverBorder, theme.hoverBorderAlpha)
        }
    }

    contentItem: Text {
        text: root.text
        font: root.font
        color: theme.foreground
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}

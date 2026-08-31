import QtQuick
import QtQuick.Controls

ToolButton {
    id: root

    font.pixelSize: theme.fontBaseSize
    implicitWidth: Math.max(theme.fontBaseSize + theme.spaceLg * 2, contentItem.implicitWidth + theme.spaceMd * 2)
    implicitHeight: theme.fontBaseSize + theme.spaceLg * 2
    padding: theme.spaceSm

    background: Rectangle {
        radius: theme.cornerRadius
        color: {
            if (root.down)
                return theme.withAlpha(theme.hoverColor, theme.pressedFillAlpha)
            if (root.visualFocus)
                return theme.withAlpha(theme.focusColor, theme.focusFillAlpha)
            if (root.hovered)
                return theme.withAlpha(theme.hoverColor, theme.hoverFillAlpha)
            return "transparent"
        }
        border.width: root.visualFocus || root.hovered ? theme.controlBorderWidth : 0
        border.color: root.visualFocus
            ? theme.withAlpha(theme.focusBorder, theme.focusBorderAlpha)
            : theme.withAlpha(theme.hoverBorder, theme.hoverBorderAlpha)
    }

    contentItem: Text {
        text: root.text
        font: root.font
        color: theme.foreground
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}

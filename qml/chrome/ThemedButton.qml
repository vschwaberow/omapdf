import QtQuick
import QtQuick.Controls

Button {
    id: root
    property bool accented: false

    font.pixelSize: theme.fontBaseSize
    leftPadding: theme.spaceLg
    rightPadding: theme.spaceLg
    topPadding: theme.spaceMd
    bottomPadding: theme.spaceMd

    background: Rectangle {
        radius: theme.cornerRadius
        color: {
            if (!root.enabled)
                return theme.withAlpha(theme.muted, theme.normalFillAlpha)
            if (root.down)
                return theme.withAlpha(theme.hoverColor, theme.pressedFillAlpha)
            if (root.visualFocus)
                return theme.withAlpha(theme.focusColor, theme.focusFillAlpha)
            if (root.hovered)
                return theme.withAlpha(theme.hoverColor, theme.hoverFillAlpha)
            if (root.accented)
                return theme.withAlpha(theme.selectedColor, theme.selectedFillAlpha)
            return theme.withAlpha(theme.hoverColor, theme.normalFillAlpha)
        }
        border.width: theme.controlBorderWidth
        border.color: {
            if (root.visualFocus)
                return theme.withAlpha(theme.focusBorder, theme.focusBorderAlpha)
            if (root.hovered)
                return theme.withAlpha(theme.hoverBorder, theme.hoverBorderAlpha)
            if (root.accented)
                return theme.withAlpha(theme.selectedBorder, theme.selectedBorderAlpha)
            return theme.withAlpha(theme.normalBorder, theme.normalBorderAlpha)
        }
    }

    contentItem: Text {
        text: root.text
        font: root.font
        color: theme.foreground
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: root.enabled ? 1.0 : 0.5
    }
}

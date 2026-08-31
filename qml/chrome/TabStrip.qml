import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: theme.darkerBackground
    clip: true
    implicitHeight: theme.fontBaseSize + theme.spaceLg * 2 + theme.spaceSm * 2
    Accessible.name: qsTr("Tabs")

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: theme.spaceSm
        anchors.rightMargin: theme.spaceSm
        anchors.topMargin: theme.spaceSm
        anchors.bottomMargin: theme.spaceSm
        spacing: theme.spaceSm

        Repeater {
            model: app.tabs
            Rectangle {
                id: tab
                required property int index
                required property string title
                required property string path
                required property int tabId

                readonly property int tabWidth: Math.min(
                    200, Math.max(88, titleMetric.width + closeBtn.implicitWidth + theme.spaceSm * 3))

                clip: true
                Layout.preferredHeight: parent.height
                Layout.preferredWidth: tab.tabWidth
                Layout.minimumWidth: tab.tabWidth
                Layout.maximumWidth: tab.tabWidth
                radius: theme.cornerRadius
                color: {
                    if (index === app.currentIndex)
                        return theme.withAlpha(theme.selectedColor, theme.selectedFillAlpha)
                    if (tabHover.hovered)
                        return theme.withAlpha(theme.hoverColor, theme.hoverFillAlpha)
                    return theme.withAlpha(theme.hoverColor, theme.normalFillAlpha)
                }
                border.color: {
                    if (index === app.currentIndex)
                        return theme.withAlpha(theme.selectedBorder, theme.selectedBorderAlpha)
                    if (tabHover.hovered)
                        return theme.withAlpha(theme.hoverBorder, theme.hoverBorderAlpha)
                    return theme.withAlpha(theme.normalBorder, theme.normalBorderAlpha)
                }
                border.width: theme.controlBorderWidth

                TextMetrics {
                    id: titleMetric
                    text: tab.title
                    font.pixelSize: theme.fontBaseSize - 2
                }

                HoverHandler { id: tabHover }
                Accessible.name: tab.title
                Accessible.role: Accessible.PageTab

                Text {
                    id: titleText
                    anchors.left: parent.left
                    anchors.right: closeBtn.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: theme.spaceSm
                    anchors.rightMargin: theme.spaceSm
                    text: tab.title
                    elide: Text.ElideMiddle
                    color: theme.foreground
                    font.pixelSize: theme.fontBaseSize - 2
                }

                ThemedToolButton {
                    id: closeBtn
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: theme.spaceSm
                    text: "×"
                    Accessible.name: qsTr("Close tab")
                    onClicked: win.requestCloseTab(index)
                    implicitWidth: theme.fontBaseSize + theme.spaceLg
                    implicitHeight: theme.fontBaseSize + theme.spaceLg
                }

                MouseArea {
                    anchors.fill: parent
                    z: -1
                    acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.MiddleButton)
                            win.requestCloseTab(index)
                        else
                            app.currentIndex = index
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        ThemedToolButton {
            text: qsTr("Open")
            Accessible.name: qsTr("Open PDF")
            onClicked: win.openFileDialog()
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: theme.darkerBackground
    clip: true
    implicitWidth: theme.fontBaseSize + theme.spaceLg * 2 + theme.spaceSm * 2
    Accessible.name: qsTr("Tools")

    readonly property int currentIndex: app.currentIndex
    readonly property var pane: {
        const _ = root.currentIndex
        const __ = app.tabs.count
        return win.activePane()
    }
    readonly property real zoomScale: pane ? pane.renderScale : 1
    readonly property bool paneDimmed: pane ? pane.dimmed : false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: theme.spaceSm
        spacing: theme.spaceSm

        RailButton {
            Layout.alignment: Qt.AlignHCenter
            glyph: "−"
            hint: qsTr("Zoom out (−)")
            enabled: root.pane !== null
            onClicked: {
                if (root.pane)
                    root.pane.zoomBy(1 / 1.1)
            }
        }

        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: theme.fontBaseSize + theme.spaceLg * 2
            Layout.preferredHeight: theme.fontBaseSize + theme.spaceLg
            Accessible.name: qsTr("Zoom percent")
            Accessible.role: Accessible.Button

            Text {
                anchors.centerIn: parent
                text: Math.round(root.zoomScale * 100)
                color: theme.foreground
                font.pixelSize: Math.max(10, theme.fontBaseSize - 3)
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.pane !== null
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (root.pane)
                        root.pane.fitWidth()
                }
                ToolTip.visible: containsMouse
                ToolTip.delay: 400
                ToolTip.text: qsTr("Fit width (w)")
            }
        }

        RailButton {
            Layout.alignment: Qt.AlignHCenter
            glyph: "+"
            hint: qsTr("Zoom in (+)")
            enabled: root.pane !== null
            onClicked: {
                if (root.pane)
                    root.pane.zoomBy(1.1)
            }
        }

        Item {
            Layout.preferredHeight: theme.spaceMd
            Layout.fillWidth: true
        }

        RailButton {
            Layout.alignment: Qt.AlignHCenter
            glyph: "↔"
            hint: qsTr("Fit width (w)")
            enabled: root.pane !== null
            onClicked: {
                if (root.pane)
                    root.pane.fitWidth()
            }
        }

        RailButton {
            Layout.alignment: Qt.AlignHCenter
            glyph: "▣"
            hint: qsTr("Fit page (0)")
            enabled: root.pane !== null
            onClicked: {
                if (root.pane)
                    root.pane.fitPage()
            }
        }

        RailButton {
            Layout.alignment: Qt.AlignHCenter
            glyph: "◐"
            hint: qsTr("Dim (d)")
            active: root.paneDimmed
            enabled: root.pane !== null
            onClicked: {
                if (!root.pane)
                    return
                root.pane.dimmed = !root.pane.dimmed
                root.pane.persistState()
            }
        }

        Item { Layout.fillHeight: true }

        RailButton {
            Layout.alignment: Qt.AlignHCenter
            glyph: "☰"
            hint: qsTr("Outline (o)")
            active: outlineDrawer.visible
            enabled: root.pane !== null
            onClicked: {
                outlineDrawer.visible = !outlineDrawer.visible
                win.outlineUserClosed = !outlineDrawer.visible
            }
        }

        RailButton {
            Layout.alignment: Qt.AlignHCenter
            glyph: "▦"
            hint: qsTr("Thumbnails (t)")
            active: win.thumbWanted
            enabled: app.tabs.count > 0
            onClicked: win.thumbWanted = !win.thumbWanted
        }
    }
}

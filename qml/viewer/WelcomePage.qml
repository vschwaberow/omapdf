import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: theme.background
    Accessible.name: qsTr("Welcome")

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: theme.darkerBackground }
            GradientStop { position: 0.45; color: theme.background }
            GradientStop { position: 1.0; color: theme.lighterBackground }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: theme.spaceLg
        width: Math.min(520, parent.width - 48)

        Image {
            source: "qrc:/qt/qml/Omapdf/assets/omapdf-logo.svg"
            fillMode: Image.PreserveAspectFit
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(520, parent.width)
            Layout.maximumWidth: Math.min(520, parent.width)
            Layout.preferredHeight: Math.min(520, parent.width) * 192 / 924
            Accessible.name: "omapdf"
            Accessible.role: Accessible.Graphic
        }
        Text {
            text: qsTr("The fastest, most beautiful PDF reading engine for Linux")
            color: theme.foreground
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: theme.fontBaseSize + 2
            Layout.fillWidth: true
        }
        Text {
            text: app.version
            color: theme.darkForeground
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: theme.fontBaseSize - 2
            Layout.fillWidth: true
        }
        Text {
            text: qsTr("Open a PDF, drop files here, or pick a recent document.")
            color: theme.darkForeground
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: theme.fontBaseSize
        }
        ThemedButton {
            text: qsTr("Open…")
            accented: true
            Layout.alignment: Qt.AlignHCenter
            Accessible.name: qsTr("Open PDF")
            onClicked: win.openFileDialog()
        }
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(220, app.recents.length * (theme.fontBaseSize + theme.spaceLg))
            model: app.recents
            clip: true
            spacing: theme.spaceSm
            Accessible.name: qsTr("Recent documents")
            delegate: ItemDelegate {
                required property string modelData
                width: ListView.view.width
                text: modelData
                Accessible.name: modelData
                onClicked: app.openRecent(modelData)
                background: Rectangle {
                    radius: theme.cornerRadius
                    color: theme.withAlpha(theme.hoverColor,
                        parent.hovered ? theme.hoverFillAlpha : theme.normalFillAlpha)
                    border.width: theme.controlBorderWidth
                    border.color: theme.withAlpha(theme.normalBorder, theme.normalBorderAlpha)
                }
                contentItem: Text {
                    text: parent.text
                    color: theme.foreground
                    elide: Text.ElideMiddle
                    font.pixelSize: theme.fontBaseSize
                }
            }
        }
    }
}

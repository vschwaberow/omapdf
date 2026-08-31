import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ThemedPopup {
    id: root
    property url url
    signal decision(bool accept)

    width: Math.min(440, Overlay.overlay ? Overlay.overlay.width - theme.spaceLg * 2 : 440)
    closePolicy: Popup.NoAutoClose
    accessibleName: qsTr("Open link?")

    onOpened: body.forceActiveFocus()
    onEscapePressed: root.decision(false)

    Label {
        Layout.fillWidth: true
        text: qsTr("Open link?")
        color: theme.foreground
        font.pixelSize: theme.fontBaseSize
        font.weight: Font.DemiBold
    }
    Label {
        Layout.fillWidth: true
        text: root.url.toString()
        wrapMode: Text.WrapAnywhere
        color: theme.darkForeground
        font.pixelSize: theme.fontBaseSize
    }
    RowLayout {
        Layout.alignment: Qt.AlignRight
        spacing: theme.spaceSm
        ThemedButton {
            text: qsTr("Cancel")
            onClicked: root.decision(false)
        }
        ThemedButton {
            text: qsTr("Open")
            accented: true
            onClicked: root.decision(true)
        }
    }
}

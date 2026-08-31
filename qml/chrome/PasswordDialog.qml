import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ThemedPopup {
    id: root
    property string targetPath: ""
    property bool retry: false
    signal acceptedPassword(string path, string password)
    signal cancelled(string path)

    closePolicy: Popup.NoAutoClose
    accessibleName: qsTr("Password required")

    function openFor(path, isRetry) {
        targetPath = path
        retry = !!isRetry
        field.text = ""
        open()
        field.forceActiveFocus()
    }

    function accept() {
        acceptedPassword(targetPath, field.text)
        close()
    }

    function reject() {
        const path = targetPath
        close()
        cancelled(path)
    }

    Label {
        Layout.fillWidth: true
        text: qsTr("Password required")
        color: theme.foreground
        font.pixelSize: theme.fontBaseSize
        font.weight: Font.DemiBold
    }
    Label {
        Layout.fillWidth: true
        visible: root.retry
        wrapMode: Text.Wrap
        text: qsTr("Wrong password. Try again.")
        color: theme.urgent
        font.pixelSize: theme.fontBaseSize
    }
    TextField {
        id: field
        Layout.fillWidth: true
        Accessible.name: qsTr("Password")
        echoMode: TextInput.Password
        placeholderText: qsTr("Password")
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
        onAccepted: root.accept()
        Keys.onEscapePressed: root.reject()
    }
    RowLayout {
        Layout.alignment: Qt.AlignRight
        spacing: theme.spaceSm
        ThemedButton {
            text: qsTr("Cancel")
            onClicked: root.reject()
        }
        ThemedButton {
            text: qsTr("Unlock")
            accented: true
            onClicked: root.accept()
        }
    }
}

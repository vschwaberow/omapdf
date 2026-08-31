import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ThemedPopup {
    id: root
    property string fileLabel: ""
    signal decided(string action)

    closePolicy: Popup.NoAutoClose
    accessibleName: qsTr("Unsaved annotations")

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: theme.foreground
        font.pixelSize: theme.fontBaseSize
        text: qsTr("Unsaved annotations in “%1”. Save before closing?").arg(root.fileLabel)
    }
    RowLayout {
        Layout.alignment: Qt.AlignRight
        spacing: theme.spaceSm
        ThemedButton {
            text: qsTr("Cancel")
            onClicked: root.decided("cancel")
        }
        ThemedButton {
            text: qsTr("Discard")
            onClicked: root.decided("discard")
        }
        ThemedButton {
            accented: true
            text: qsTr("Save")
            onClicked: root.decided("save")
        }
    }
}

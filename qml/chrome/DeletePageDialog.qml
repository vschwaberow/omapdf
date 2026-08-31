import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ThemedPopup {
    id: root
    property int pageNumber: 1
    signal decided(bool accepted)

    closePolicy: Popup.CloseOnEscape
    accessibleName: qsTr("Delete page")

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: theme.foreground
        font.pixelSize: theme.fontBaseSize
        text: qsTr("Delete page %1 from the PDF file? This writes the file immediately.").arg(root.pageNumber)
    }
    RowLayout {
        Layout.alignment: Qt.AlignRight
        spacing: theme.spaceSm
        ThemedButton {
            text: qsTr("Cancel")
            onClicked: root.decided(false)
        }
        ThemedButton {
            text: qsTr("Delete")
            accented: true
            onClicked: root.decided(true)
        }
    }
}

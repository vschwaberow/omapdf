import QtQuick
import QtQuick.Controls

Rectangle {
    property alias text: label.text
    color: theme.darkerBackground
    Accessible.name: qsTr("Status")

    Text {
        id: label
        anchors.fill: parent
        anchors.leftMargin: theme.spaceMd
        anchors.rightMargin: theme.spaceMd
        color: theme.darkForeground
        font.pixelSize: Math.max(11, theme.fontBaseSize - 2)
        elide: Text.ElideMiddle
        verticalAlignment: Text.AlignVCenter
    }
}

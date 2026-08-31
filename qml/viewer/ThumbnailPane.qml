import QtQuick
import QtQuick.Controls
import QtQuick.Pdf

Rectangle {
    id: root
    property PdfDocument document: null
    property int currentPage: 0
    property int displayedPage: 0
    color: theme.darkerBackground
    Accessible.name: qsTr("Thumbnails")
    signal navigate(int page)

    Rectangle {
        anchors.right: parent.right
        width: theme.controlBorderWidth
        height: parent.height
        color: theme.withAlpha(theme.normalBorder, theme.normalBorderAlpha)
    }

    onCurrentPageChanged: followDebounce.restart()
    onDisplayedPageChanged: Qt.callLater(root.positionViewAtCurrent)

    Timer {
        id: followDebounce
        interval: 80
        onTriggered: root.displayedPage = root.currentPage
    }
    onDocumentChanged: Qt.callLater(root.positionViewAtCurrent)

    function positionViewAtCurrent() {
        if (!list.count)
            return
        if (root.displayedPage >= 0 && root.displayedPage < list.count)
            list.positionViewAtIndex(root.displayedPage, ListView.Contain)
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: theme.spaceSm
        clip: true
        spacing: theme.spaceSm
        boundsBehavior: Flickable.StopAtBounds
        model: (root.visible && root.document
                && root.document.status === PdfDocument.Ready)
               ? root.document.pageCount : 0
        cacheBuffer: Math.max(0, Math.round(height * 2))
        reuseItems: true
        onCountChanged: Qt.callLater(root.positionViewAtCurrent)

        delegate: Item {
            id: del
            required property int index
            width: list.width
            height: Math.round(width * 1.35) + theme.fontBaseSize + theme.spaceSm

            readonly property bool selected: index === root.displayedPage

            Rectangle {
                anchors.fill: parent
                radius: theme.cornerRadius
                color: {
                    if (del.selected)
                        return theme.withAlpha(theme.selectedColor, theme.selectedFillAlpha)
                    if (area.containsMouse)
                        return theme.withAlpha(theme.hoverColor, theme.hoverFillAlpha)
                    return theme.withAlpha(theme.hoverColor, theme.normalFillAlpha)
                }
                border.width: theme.controlBorderWidth
                border.color: del.selected
                    ? theme.withAlpha(theme.selectedBorder, theme.selectedBorderAlpha)
                    : theme.withAlpha(theme.normalBorder, theme.normalBorderAlpha)

                Column {
                    anchors.fill: parent
                    anchors.margins: theme.spaceSm
                    spacing: theme.spaceSm

                    PdfPageImage {
                        id: image
                        width: parent.width
                        height: Math.round(width * 1.3)
                        document: root.document
                        currentFrame: del.index
                        asynchronous: true
                        fillMode: Image.PreserveAspectFit
                        sourceSize.width: Math.min(256, Math.round(width * Screen.devicePixelRatio))
                        sourceSize.height: 0
                    }
                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: (del.index + 1).toString()
                        color: theme.darkForeground
                        font.pixelSize: theme.fontBaseSize - 2
                    }
                }
            }

            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.navigate(del.index)
            }

            Accessible.name: qsTr("Page %1").arg(del.index + 1)
            Accessible.role: Accessible.ListItem
            Accessible.onPressAction: root.navigate(del.index)
        }
    }
}

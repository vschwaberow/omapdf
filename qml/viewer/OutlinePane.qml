import QtQuick
import QtQuick.Controls
import QtQuick.Pdf
import QtQml.Models

Rectangle {
    id: root
    property PdfDocument document: null
    color: theme.darkerBackground
    Accessible.name: qsTr("Outline")
    signal navigate(int page)

    Rectangle {
        anchors.right: parent.right
        width: theme.controlBorderWidth
        height: parent.height
        color: theme.withAlpha(theme.normalBorder, theme.normalBorderAlpha)
    }

    Loader {
        anchors.fill: parent
        anchors.margins: theme.spaceSm
        active: root.document !== null
        sourceComponent: Component {
            TreeView {
                id: tree
                model: PdfBookmarkModel {
                    document: root.document
                }
                clip: true
                selectionModel: ItemSelectionModel {}
                rowSpacing: theme.spaceSm
                boundsBehavior: Flickable.StopAtBounds

                delegate: TreeViewDelegate {
                    id: del
                    required property string title
                    required property int page
                    implicitHeight: theme.fontBaseSize + theme.spaceMd * 2
                    leftPadding: theme.spaceSm
                    indentation: theme.spaceLg
                    onClicked: root.navigate(page)

                    contentItem: Text {
                        text: del.title
                        color: theme.foreground
                        font.pixelSize: theme.fontBaseSize
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: theme.cornerRadius
                        color: {
                            if (del.highlighted || del.down)
                                return theme.withAlpha(theme.selectedColor, theme.selectedFillAlpha)
                            if (del.hovered)
                                return theme.withAlpha(theme.hoverColor, theme.hoverFillAlpha)
                            return theme.withAlpha(theme.hoverColor, theme.normalFillAlpha)
                        }
                        border.width: del.highlighted ? theme.controlBorderWidth : 0
                        border.color: theme.withAlpha(theme.selectedBorder, theme.selectedBorderAlpha)
                    }
                }
            }
        }
    }
}

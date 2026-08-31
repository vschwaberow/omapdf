import QtQuick
import QtQuick.Controls
import QtQuick.Pdf

Item {
    id: root
    required property string path
    property int tabIndex: -1
    Accessible.name: {
        const _ = doc.status
        return docTitle()
    }
    Accessible.role: Accessible.Document
    property bool dimmed: false
    property alias document: doc
    property int bookmarkCount: 0
    property alias annots: annotStore
    property bool dirty: annotStore.dirty
    property alias currentPage: view.currentPage
    readonly property real renderScale: view.renderScale
    property int pendingRestorePage: -1
    property real pendingRestoreZoom: 0
    property bool structureReload: false
    property bool structureAwaiting: false
    property bool structureNeedsReload: false
    property string structureOkMessage: ""
    property int structurePendingPage: -1
    property real structurePendingZoom: 0
    property string fitMode: ""
    property bool applyingFit: false

    signal passwordNeeded(bool retry)
    signal status(string msg)
    signal bookmarksAvailable(int count)
    signal searchStatsChanged()
    signal loadFailed()
    signal dirtyCloseRequested()
    signal requestDeletePage()
    signal requestExtract()
    signal requestMerge()

    readonly property string statusLine: {
        if (doc.status !== PdfDocument.Ready)
            return path
        const dirtyMark = annotStore.dirty ? " *" : ""
        return (docTitle() + dirtyMark + " · " + (view.currentPage + 1) + "/" + doc.pageCount
                + " · " + Math.round(view.renderScale * 100) + "%")
    }

    AnnotStore {
        id: annotStore
    }

    function docTitle() {
        const t = doc.title
        if (t && String(t).length)
            return String(t)
        return path.split("/").pop()
    }

    function documentUrl() {
        return app.fileUrl(path)
    }

    function openDocument() {
        doc.source = documentUrl()
    }

    function closeDocument() {
        doc.source = ""
    }

    readonly property int searchHitCount: view.searchModel.count
    readonly property int searchHitIndex: view.searchModel.currentResult < 0
        ? 0
        : view.searchModel.currentResult + 1

    function setSearch(text) { view.searchString = text }
    function searchText() { return view.searchString }
    function searchForward() { view.searchForward() }
    function searchBack() { view.searchBack() }
    function zoomBy(factor) {
        fitMode = ""
        view.renderScale = Math.max(0.25, Math.min(4, view.renderScale * factor))
    }
    function fitWidth() {
        fitMode = "width"
        applyFitMode()
    }
    function fitPage() {
        fitMode = "page"
        applyFitMode()
    }
    function applyFitMode() {
        if (!fitMode.length || root.width < 1 || root.height < 1)
            return
        applyingFit = true
        if (fitMode === "width")
            view.scaleToWidth(root.width, root.height)
        else if (fitMode === "page")
            view.scaleToPage(root.width, root.height)
        applyingFit = false
    }
    function goToPage(page) { view.goToPage(page) }
    function reloadFromDisk() {
        if (annotStore.dirty) {
            root.status(qsTr("Save or discard annotations before reload"))
            return
        }
        if (doc.status === PdfDocument.Ready) {
            persistState()
            pendingRestorePage = view.currentPage
            pendingRestoreZoom = view.renderScale
            structureReload = true
        }
        closeDocument()
        openDocument()
    }
    function navBack() {
        if (view.backEnabled)
            view.back()
    }
    function navForward() {
        if (view.forwardEnabled)
            view.forward()
    }
    function scrollBy(deltaPages) {
        const next = Math.max(0, Math.min(doc.pageCount - 1, view.currentPage + deltaPages))
        view.goToPage(next)
    }
    function scrollViewport(direction) {
        const page = Math.max(48, root.height * 0.9)
        const maxY = Math.max(0, view.contentHeight - root.height)
        view.setContentY(Math.max(0, Math.min(maxY, view.contentY + direction * page)))
    }
    function copySelection() {
        if (view.copySelectionToClipboard())
            return
        if (view.selectedText.length) {
            app.copyText(view.selectedText)
            root.status(qsTr("Copied"))
        }
    }
    function selectAll() { view.selectAll() }

    Connections {
        target: view
        function onCopySucceeded() { root.status(qsTr("Copied")) }
    }
    function setPassword(password) {
        doc.password = password
        closeDocument()
        openDocument()
    }
    function persistState() {
        app.saveState(path, view.renderScale, view.currentPage, view.contentY, root.dimmed)
    }
    function saveAnnots() {
        if (annotStore.save())
            root.status(qsTr("Annotations saved"))
        else
            root.status(qsTr("Failed to save annotations"))
    }
    function undoAnnot() {
        if (annotStore.canUndo())
            annotStore.undo()
    }
    function redoAnnot() {
        if (annotStore.canRedo())
            annotStore.redo()
    }
    function highlightSelection() {
        const cap = view.captureSelection()
        if (!cap) {
            root.status(qsTr("Select text to highlight"))
            return
        }
        annotStore.addHighlight(cap.page, cap.text, cap.geometry)
        root.status(qsTr("Highlight added"))
    }
    function beginNote() {
        notePopup.page = view.lastTapPage >= 0 ? view.lastTapPage : view.currentPage
        notePopup.nx = view.lastTapPage >= 0 ? view.lastTapX : 72
        notePopup.ny = view.lastTapPage >= 0 ? view.lastTapY : 72
        notePopup.text = ""
        notePopup.open()
    }
    function setAnnotColor(color) {
        annotStore.activeColor = color
        app.setAnnotColor(annotStore.activeColor)
    }
    function enforceLimits() {
        if (doc.pageCount > omapdfMaxPageCount) {
            root.status(qsTr("Rejected: too many pages"))
            closeDocument()
            return false
        }
        return true
    }
    function loadAnnots() {
        annotStore.load(path, annotStore.contentHash(path))
    }

    function beginStructure(okMessage, needsReload, restorePage, restoreZoom) {
        if (needsReload) {
            if (doc.status !== PdfDocument.Ready)
                return false
            if (annotStore.dirty) {
                root.status(qsTr("Save or discard annotations before structure ops"))
                return false
            }
        }
        if (structure.busy || structureAwaiting) {
            root.status(qsTr("Structure op already in progress"))
            return false
        }
        if (needsReload) {
            persistState()
            structurePendingPage = restorePage
            structurePendingZoom = restoreZoom
            closeDocument()
        }
        structureAwaiting = true
        structureNeedsReload = needsReload
        structureOkMessage = okMessage
        return true
    }

    function beginStructureReload(okMessage, restorePage, restoreZoom) {
        return beginStructure(okMessage, true, restorePage, restoreZoom)
    }

    function beginStructureStatus(okMessage) {
        return beginStructure(okMessage, false, 0, 0)
    }

    function finishStructure(result) {
        structureAwaiting = false
        if (structureNeedsReload) {
            structureNeedsReload = false
            if (!result || !result.ok) {
                openDocument()
                root.status((result && result.error) ? result.error : qsTr("Structure op failed"))
                return
            }
            pendingRestorePage = structurePendingPage
            pendingRestoreZoom = structurePendingZoom
            structureReload = true
            openDocument()
            root.status(structureOkMessage)
            return
        }
        if (!result || !result.ok)
            root.status((result && result.error) ? result.error : qsTr("Structure op failed"))
        else
            root.status(structureOkMessage)
    }

    function rotatePage(degrees) {
        const page = view.currentPage
        const zoom = view.renderScale
        if (!beginStructureReload(qsTr("Page rotated"), page, zoom))
            return
        structure.rotateAsync(path, page, degrees)
    }

    function deleteCurrentPage() {
        if (doc.pageCount <= 1) {
            root.status(qsTr("Cannot delete the only page"))
            return
        }
        const page = view.currentPage
        const zoom = view.renderScale
        if (!beginStructureReload(qsTr("Page deleted"), page, zoom))
            return
        structure.removePagesAsync(path, [page])
    }

    function movePage(delta) {
        const count = doc.pageCount
        const page = view.currentPage
        const target = page + delta
        if (target < 0 || target >= count)
            return
        const order = []
        for (let i = 0; i < count; ++i)
            order.push(i)
        order[page] = target
        order[target] = page
        const zoom = view.renderScale
        if (!beginStructureReload(qsTr("Page moved"), target, zoom))
            return
        structure.reorderAsync(path, order)
    }

    function extractPagesTo(dest) {
        if (!beginStructureStatus(qsTr("Page extracted")))
            return
        structure.extractAsync(path, [view.currentPage], dest)
    }

    function mergeFrom(sources, dest) {
        if (!beginStructureStatus(qsTr("Merged PDF written")))
            return
        structure.mergeAsync(sources, dest)
    }

    function exportAnnotsTo(dest) {
        if (annotStore.rowCount() === 0) {
            root.status(qsTr("No annotations to export"))
            return
        }
        if (!beginStructureStatus(qsTr("Annotations exported to PDF")))
            return
        structure.exportAnnotsAsync(path, dest, annotStore.items())
    }

    PdfDocument {
        id: doc
        onPasswordRequired: root.passwordNeeded(doc.password.length > 0)
        onStatusChanged: function(status) {
            if (status === PdfDocument.Ready) {
                if (!root.enforceLimits())
                    return
                root.loadAnnots()
                app.noteOpened(path)
                Qt.callLater(() => {
                    const title = root.docTitle()
                    if (title.length)
                        app.setTabTitle(tabIndex, title)
                })
                if (root.structureReload) {
                    root.structureReload = false
                    if (root.pendingRestoreZoom > 0) {
                        fitMode = ""
                        view.renderScale = root.pendingRestoreZoom
                    }
                    root.pendingRestoreZoom = 0
                    const page = Math.max(0, Math.min(root.pendingRestorePage, doc.pageCount - 1))
                    root.pendingRestorePage = -1
                    Qt.callLater(() => view.goToPage(page))
                    Qt.callLater(() => {
                        root.bookmarkCount = bookmarkModel.rowCount()
                        root.bookmarksAvailable(root.bookmarkCount)
                    })
                    return
                }
                const st = app.loadState(path)
                if (st.zoom && st.zoom > 0) {
                    fitMode = ""
                    view.renderScale = st.zoom
                } else {
                    Qt.callLater(() => root.fitWidth())
                }
                const restorePage = (st.page !== undefined && st.page !== null) ? st.page : -1
                const restoreScrollY = st.scrollY || 0
                if (restorePage >= 0) {
                    Qt.callLater(() => {
                        view.goToPage(restorePage)
                        if (restoreScrollY > 0)
                            Qt.callLater(() => view.setContentY(restoreScrollY))
                    })
                } else if (restoreScrollY > 0) {
                    Qt.callLater(() => view.setContentY(restoreScrollY))
                }
                if (st.dimmed)
                    root.dimmed = true
                Qt.callLater(() => {
                    root.bookmarkCount = bookmarkModel.rowCount()
                    root.bookmarksAvailable(root.bookmarkCount)
                })
            } else if (status === PdfDocument.Error) {
                const errText = String(doc.error || "")
                if (errText.toLowerCase().indexOf("password") >= 0) {
                    root.passwordNeeded(true)
                    return
                }
                root.status(qsTr("Failed to open"))
                root.loadFailed()
            }
        }
    }

    PdfBookmarkModel {
        id: bookmarkModel
        document: doc
    }

    OmapdfMultiPageView {
        id: view
        anchors.fill: parent
        document: doc
        annotStore: annotStore
    }

    WheelHandler {
        acceptedModifiers: Qt.ControlModifier
        onWheel: (event) => {
            if (event.angleDelta.y > 0)
                root.zoomBy(1.1)
            else if (event.angleDelta.y < 0)
                root.zoomBy(1 / 1.1)
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.BackButton | Qt.ForwardButton
        onClicked: (mouse) => {
            if (mouse.button === Qt.BackButton)
                root.navBack()
            else if (mouse.button === Qt.ForwardButton)
                root.navForward()
        }
    }

    onWidthChanged: Qt.callLater(applyFitMode)
    onHeightChanged: Qt.callLater(applyFitMode)

    Connections {
        target: view
        function onRenderScaleChanged() {
            if (!root.applyingFit && root.fitMode.length)
                root.fitMode = ""
        }
    }

    Connections {
        target: view.searchModel
        function onCountChanged() { root.searchStatsChanged() }
        function onCurrentResultChanged() { root.searchStatsChanged() }
    }

    Rectangle {
        anchors.fill: parent
        color: theme.darkerBackground
        opacity: root.dimmed ? 0.35 : 0
        visible: opacity > 0
        z: 10
        enabled: false
    }

    Popup {
        id: notePopup
        property int page: 0
        property real nx: 0
        property real ny: 0
        property alias text: noteField.text
        modal: true
        anchors.centerIn: parent
        width: Math.min(360, root.width - theme.spaceLg * 5)
        padding: theme.spaceLg
        background: Rectangle {
            color: theme.darkBackground
            border.color: theme.withAlpha(theme.focusBorder, theme.focusBorderAlpha)
            border.width: theme.controlBorderWidth
            radius: theme.cornerRadius
        }
        Column {
            width: parent.width
            spacing: theme.spaceMd
            Label {
                text: qsTr("Note")
                color: theme.foreground
                font.pixelSize: theme.fontBaseSize
            }
            TextField {
                id: noteField
                width: parent.width
                focus: true
                color: theme.foreground
                font.pixelSize: theme.fontBaseSize
                background: Rectangle {
                    radius: theme.cornerRadius
                    color: theme.withAlpha(theme.hoverColor,
                        noteField.activeFocus ? theme.focusFillAlpha : theme.normalFillAlpha)
                    border.width: theme.controlBorderWidth
                    border.color: noteField.activeFocus
                        ? theme.withAlpha(theme.focusBorder, theme.focusBorderAlpha)
                        : theme.withAlpha(theme.normalBorder, theme.normalBorderAlpha)
                }
                onAccepted: notePopup.accept()
            }
            Row {
                spacing: theme.spaceSm
                ThemedButton {
                    text: qsTr("Add")
                    accented: true
                    onClicked: notePopup.accept()
                }
                ThemedButton {
                    text: qsTr("Cancel")
                    onClicked: notePopup.close()
                }
            }
        }
        function accept() {
            if (noteField.text.trim().length === 0) {
                close()
                return
            }
            annotStore.addNote(page, nx, ny, noteField.text)
            root.status(qsTr("Note added"))
            close()
        }
    }

    Connections {
        target: structure
        function onOpFinished(result) {
            if (!root.structureAwaiting)
                return
            root.finishStructure(result)
        }
    }

    onEnabledChanged: {
        if (enabled)
            openDocument()
        else {
            persistState()
            closeDocument()
        }
    }

    Component.onCompleted: {
        annotStore.activeColor = app.annotColor()
        if (enabled)
            openDocument()
    }

    Component.onDestruction: persistState()
}

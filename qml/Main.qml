import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: win
    font.pixelSize: theme.fontBaseSize
    width: 1200
    height: 800
    visible: true
    title: currentTitle()
    color: theme.background

    property string statusText: ""
    property bool outlineUserClosed: false
    property bool thumbWanted: false
    property int thumbUserWidth: 112
    property int outlineUserWidth: 200
    readonly property int sideChromeBudget: Math.max(200, Math.floor(width * 0.28))
    readonly property bool thumbPaneVisible: thumbWanted && app.tabs.count > 0
    readonly property int thumbMinWidth: 80
    readonly property int thumbMaxWidth: 160
    readonly property int outlineMinWidth: thumbPaneVisible ? 100 : 140
    readonly property int outlineMaxWidth: 320

    function applySidePreferredWidths() {
        if (mainSplit.resizing)
            return
        const budget = sideChromeBudget
        let t = thumbPaneVisible ? thumbUserWidth : 0
        let o = outlineDrawer.visible ? outlineUserWidth : 0
        if (t > 0)
            t = Math.min(Math.max(t, thumbMinWidth), Math.min(thumbMaxWidth, budget))
        if (o > 0) {
            const oMax = Math.min(outlineMaxWidth, Math.max(outlineMinWidth, budget - t))
            o = Math.min(Math.max(o, outlineMinWidth), oMax)
        }
        if (t + o > budget) {
            if (o > 0)
                o = Math.max(outlineMinWidth, budget - t)
            if (t + o > budget && t > 0)
                t = Math.max(thumbMinWidth, budget - o)
        }
        if (thumbPaneVisible) {
            thumbUserWidth = t
            thumbDrawer.SplitView.preferredWidth = t
        }
        if (outlineDrawer.visible) {
            outlineUserWidth = o
            outlineDrawer.SplitView.preferredWidth = o
        }
    }

    onWidthChanged: Qt.callLater(applySidePreferredWidths)

    Timer {
        id: statusClear
        interval: 4000
        onTriggered: win.statusText = ""
    }
    onStatusTextChanged: {
        if (statusText.length)
            statusClear.restart()
        else
            statusClear.stop()
    }
    property var mergePendingSources: []

    function currentTitle() {
        if (app.currentIndex < 0 || app.currentIndex >= app.tabs.count)
            return qsTr("omapdf") + " " + app.version
        return app.tabTitleAt(app.currentIndex) + " — omapdf"
    }

    function openFileDialog() { fileDialog.open() }

    function activePane() {
        if (app.currentIndex < 0)
            return null
        return pdfRepeater.itemAt(app.currentIndex)
    }

    function searchStep(forward) {
        const p = activePane()
        if (!p)
            return
        if (!p.searchText().length) {
            searchBar.forceActiveFocus()
            return
        }
        if (forward)
            p.searchForward()
        else
            p.searchBack()
    }

    function setPaletteColor(index) {
        const p = activePane()
        if (p && p.annots.palette.length > index)
            p.setAnnotColor(p.annots.palette[index])
    }

    function refreshSearchHits() {
        const p = activePane()
        if (!p) {
            searchBar.hitCount = 0
            searchBar.hitIndex = 0
            return
        }
        searchBar.hitCount = p.searchHitCount
        searchBar.hitIndex = p.searchHitIndex
    }

    function syncOutlineForActiveTab() {
        const p = activePane()
        if (!p || p.bookmarkCount <= 0) {
            outlineDrawer.visible = false
            return
        }
        if (!outlineUserClosed)
            outlineDrawer.visible = true
    }

    property int pendingCloseIndex: -1
    property int previousTabIndex: -1

    function requestCloseTab(index) {
        if (index < 0)
            return
        const p = pdfRepeater.itemAt(index)
        if (p && p.dirty) {
            pendingCloseIndex = index
            dirtyDialog.fileLabel = app.tabTitleAt(index)
            dirtyDialog.open()
            return
        }
        if (p)
            p.persistState()
        app.closeTab(index)
        previousTabIndex = app.currentIndex
    }

    property bool closingWindow: false

    function finishDirtyClose(action) {
        const index = pendingCloseIndex
        dirtyDialog.close()
        pendingCloseIndex = -1
        if (index < 0)
            return
        const p = pdfRepeater.itemAt(index)
        if (action === "cancel") {
            closingWindow = false
            return
        }
        if (action === "save" && p) {
            p.saveAnnots()
            if (p.dirty) {
                closingWindow = false
                return
            }
        }
        if (action === "discard" && p)
            p.annots.discard()
        if (closingWindow) {
            if (p)
                p.persistState()
            closingWindow = false
            Qt.callLater(() => win.close())
            return
        }
        if (p)
            p.persistState()
        app.closeTab(index)
        previousTabIndex = app.currentIndex
    }

    Shortcut {
        sequences: [StandardKey.Open]
        onActivated: openFileDialog()
    }
    Shortcut {
        sequence: "o"
        enabled: !searchBar.fieldFocus
        onActivated: openFileDialog()
    }
    Shortcut {
        sequences: [StandardKey.Close]
        onActivated: requestCloseTab(app.currentIndex)
    }
    Shortcut {
        sequences: [StandardKey.Quit]
        onActivated: win.close()
    }
    Shortcut {
        sequences: [StandardKey.Refresh, "F5"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus && !structure.busy
        onActivated: { const p = activePane(); if (p) p.reloadFromDisk() }
    }
    Shortcut {
        sequence: "Ctrl+Tab"
        onActivated: {
            if (app.tabs.count === 0) return
            app.currentIndex = (app.currentIndex + 1) % app.tabs.count
        }
    }
    Shortcut {
        sequence: "Ctrl+Shift+Tab"
        onActivated: {
            if (app.tabs.count === 0) return
            app.currentIndex = (app.currentIndex - 1 + app.tabs.count) % app.tabs.count
        }
    }
    Shortcut {
        sequences: ["/", StandardKey.Find]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: searchBar.forceActiveFocus()
    }
    Shortcut {
        sequences: ["n", "F3"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: searchStep(true)
    }
    Shortcut {
        sequences: ["Shift+N", "Shift+F3"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: searchStep(false)
    }
    Shortcut {
        sequence: "Ctrl+B"
        enabled: app.currentIndex >= 0
        onActivated: {
            outlineDrawer.visible = !outlineDrawer.visible
            outlineUserClosed = !outlineDrawer.visible
        }
    }
    Shortcut {
        sequences: ["t", "Ctrl+T"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: win.thumbWanted = !win.thumbWanted
    }
    Shortcut {
        sequences: ["+", "=", StandardKey.ZoomIn]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.zoomBy(1.1) }
    }
    Shortcut {
        sequences: ["-", StandardKey.ZoomOut]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.zoomBy(1 / 1.1) }
    }
    Shortcut {
        sequence: "w"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.fitWidth() }
    }
    Shortcut {
        sequence: "0"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.fitPage() }
    }
    Shortcut {
        sequence: "d"
        enabled: !searchBar.fieldFocus
        onActivated: {
            const p = activePane()
            if (!p)
                return
            p.dimmed = !p.dimmed
            p.persistState()
        }
    }
    Shortcut {
        sequences: [StandardKey.Print]
        enabled: app.currentIndex >= 0
        onActivated: app.printPdf(app.tabPathAt(app.currentIndex))
    }
    Shortcut {
        sequences: ["j", "PgDown"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.scrollBy(1) }
    }
    Shortcut {
        sequences: ["k", "PgUp"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.scrollBy(-1) }
    }
    Shortcut {
        sequence: "Space"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.scrollViewport(1) }
    }
    Shortcut {
        sequence: "Shift+Space"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.scrollViewport(-1) }
    }
    Shortcut {
        sequences: ["g", "Home"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.goToPage(0) }
    }
    Shortcut {
        sequences: ["G", "End"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: {
            const p = activePane()
            if (p && p.document)
                p.goToPage(Math.max(0, p.document.pageCount - 1))
        }
    }
    Shortcut {
        sequences: [StandardKey.Back, "Alt+Left"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.navBack() }
    }
    Shortcut {
        sequences: [StandardKey.Forward, "Alt+Right"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.navForward() }
    }
    Shortcut {
        sequences: [StandardKey.Copy, "Ctrl+C"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.copySelection() }
    }
    Shortcut {
        sequences: [StandardKey.SelectAll]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.selectAll() }
    }
    Shortcut {
        sequences: [StandardKey.Save]
        enabled: app.currentIndex >= 0
        onActivated: { const p = activePane(); if (p) p.saveAnnots() }
    }
    Shortcut {
        sequences: [StandardKey.Undo, "Ctrl+Z"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.undoAnnot() }
    }
    Shortcut {
        sequences: [StandardKey.Redo, "Ctrl+Y", "Ctrl+Shift+Z"]
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.redoAnnot() }
    }
    Shortcut {
        sequence: "h"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.highlightSelection() }
    }
    Shortcut {
        sequence: "a"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: { const p = activePane(); if (p) p.beginNote() }
    }
    Shortcut {
        sequence: "1"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: setPaletteColor(0)
    }
    Shortcut {
        sequence: "2"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: setPaletteColor(1)
    }
    Shortcut {
        sequence: "3"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: setPaletteColor(2)
    }
    Shortcut {
        sequence: "4"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus
        onActivated: setPaletteColor(3)
    }
    Shortcut {
        sequence: "r"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus && !structure.busy
        onActivated: { const p = activePane(); if (p) p.rotatePage(90) }
    }
    Shortcut {
        sequence: "Shift+R"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus && !structure.busy
        onActivated: { const p = activePane(); if (p) p.rotatePage(-90) }
    }
    Shortcut {
        sequence: "x"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus && !structure.busy
        onActivated: {
            const p = activePane()
            if (!p)
                return
            deletePageDialog.pageNumber = p.currentPage + 1
            deletePageDialog.open()
        }
    }
    Shortcut {
        sequence: "["
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus && !structure.busy
        onActivated: { const p = activePane(); if (p) p.movePage(-1) }
    }
    Shortcut {
        sequence: "]"
        enabled: app.currentIndex >= 0 && !searchBar.fieldFocus && !structure.busy
        onActivated: { const p = activePane(); if (p) p.movePage(1) }
    }
    Shortcut {
        sequence: "Ctrl+E"
        enabled: app.currentIndex >= 0 && !structure.busy
        onActivated: extractDialog.open()
    }
    Shortcut {
        sequence: "Ctrl+Shift+M"
        enabled: app.currentIndex >= 0 && !structure.busy
        onActivated: mergeSourceDialog.open()
    }
    Shortcut {
        sequence: "Ctrl+Shift+S"
        enabled: app.currentIndex >= 0 && !structure.busy
        onActivated: exportAnnotDialog.open()
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Open PDF")
        nameFilters: [qsTr("PDF files (*.pdf)"), qsTr("All files (*)")]
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            for (let i = 0; i < selectedFiles.length; ++i)
                app.openUrl(selectedFiles[i])
        }
    }

    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: (drop) => {
            if (!drop.hasUrls)
                return
            let opened = 0
            for (let i = 0; i < drop.urls.length; ++i) {
                const u = drop.urls[i]
                if (String(u).toLowerCase().endsWith(".pdf")) {
                    app.openUrl(u)
                    opened++
                }
            }
            if (opened === 0)
                win.statusText = qsTr("Drop PDF files to open")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabStrip {
            z: 1
            Layout.fillWidth: true
            Layout.preferredHeight: theme.fontBaseSize + theme.spaceLg * 2
                                           + theme.spaceSm * 2
        }

        SearchBar {
            id: searchBar
            z: 1
            Layout.fillWidth: true
            visible: app.tabs.count > 0
            onSearchChanged: (text) => {
                const p = activePane()
                if (p)
                    p.setSearch(text)
                Qt.callLater(refreshSearchHits)
            }
            onForward: { const p = activePane(); if (p) p.searchForward() }
            onBack: { const p = activePane(); if (p) p.searchBack() }
        }

        Connections {
            target: app
            function onCurrentIndexChanged() {
                if (win.previousTabIndex >= 0
                        && win.previousTabIndex !== app.currentIndex) {
                    const prev = pdfRepeater.itemAt(win.previousTabIndex)
                    if (prev)
                        prev.persistState()
                }
                win.previousTabIndex = app.currentIndex
                const p = activePane()
                searchBar.setText(p ? p.searchText() : "")
                refreshSearchHits()
                syncOutlineForActiveTab()
            }
            function onTabsChanged() {
                if (app.tabs.count === 0)
                    win.thumbWanted = false
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            SplitView {
                id: mainSplit
                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: Qt.Horizontal

                handle: Rectangle {
                    implicitWidth: 8
                    implicitHeight: 8
                    color: SplitHandle.pressed || SplitHandle.hovered
                           ? theme.withAlpha(theme.hoverBorder, theme.hoverBorderAlpha)
                           : theme.withAlpha(theme.normalBorder, theme.normalBorderAlpha)
                }

                onResizingChanged: {
                    if (resizing)
                        return
                    if (thumbDrawer.visible && thumbDrawer.width > 0)
                        win.thumbUserWidth = Math.round(thumbDrawer.width)
                    if (outlineDrawer.visible && outlineDrawer.width > 0)
                        win.outlineUserWidth = Math.round(outlineDrawer.width)
                    win.applySidePreferredWidths()
                }

                ThumbnailPane {
                    id: thumbDrawer
                    visible: win.thumbPaneVisible
                    SplitView.minimumWidth: win.thumbMinWidth
                    SplitView.maximumWidth: win.thumbMaxWidth
                    document: {
                        const p = activePane()
                        return p ? p.document : null
                    }
                    currentPage: {
                        const p = activePane()
                        return p ? p.currentPage : 0
                    }
                    onNavigate: (page) => { const p = activePane(); if (p) p.goToPage(page) }
                    onVisibleChanged: Qt.callLater(win.applySidePreferredWidths)
                }

                OutlinePane {
                    id: outlineDrawer
                    visible: false
                    SplitView.minimumWidth: win.outlineMinWidth
                    SplitView.maximumWidth: win.outlineMaxWidth
                    document: {
                        const p = activePane()
                        return p ? p.document : null
                    }
                    onNavigate: (page) => { const p = activePane(); if (p) p.goToPage(page) }
                    onVisibleChanged: Qt.callLater(win.applySidePreferredWidths)
                }

                Item {
                    clip: true
                    SplitView.fillWidth: true
                    SplitView.minimumWidth: 280

                    WelcomePage {
                        anchors.fill: parent
                        visible: app.tabs.count === 0
                    }

                    Repeater {
                        id: pdfRepeater
                        model: app.tabs
                        PdfPane {
                            required property int index
                            required property int tabId
                            anchors.fill: parent
                            visible: index === app.currentIndex
                            enabled: index === app.currentIndex
                            tabIndex: index
                            onPasswordNeeded: (retry) => passwordDialog.openFor(path, retry)
                            onStatus: (msg) => win.statusText = msg
                            onLoadFailed: {
                                const i = index
                                Qt.callLater(() => app.closeTab(i))
                            }
                            onSearchStatsChanged: {
                                if (index === app.currentIndex)
                                    win.refreshSearchHits()
                            }
                            onBookmarksAvailable: (count) => {
                                if (index !== app.currentIndex)
                                    return
                                if (count > 0 && !win.outlineUserClosed)
                                    outlineDrawer.visible = true
                                else if (count === 0)
                                    outlineDrawer.visible = false
                            }
                        }
                    }
                }
            }

            ToolRail {
                id: toolRail
                z: 1
                visible: app.tabs.count > 0
                Layout.fillHeight: true
                Layout.preferredWidth: implicitWidth
                Layout.maximumWidth: implicitWidth
            }
        }

        StatusBar {
            Layout.fillWidth: true
            Layout.preferredHeight: theme.fontBaseSize + theme.spaceLg
            text: {
                if (app.currentIndex < 0)
                    return win.statusText
                const p = activePane()
                if (!p)
                    return win.statusText
                const line = p.statusLine
                return win.statusText.length ? (line + " · " + win.statusText) : line
            }
        }
    }

    DirtyCloseDialog {
        id: dirtyDialog
        onDecided: (action) => finishDirtyClose(action)
    }

    DeletePageDialog {
        id: deletePageDialog
        onDecided: (ok) => {
            deletePageDialog.close()
            if (!ok)
                return
            const p = activePane()
            if (p)
                p.deleteCurrentPage()
        }
    }

    FileDialog {
        id: extractDialog
        title: qsTr("Extract page as PDF")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PDF files (*.pdf)")]
        defaultSuffix: "pdf"
        onAccepted: {
            const p = activePane()
            if (p && selectedFile)
                p.extractPagesTo(app.localPath(selectedFile))
        }
    }

    FileDialog {
        id: mergeSourceDialog
        title: qsTr("Merge PDFs — choose sources")
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("PDF files (*.pdf)")]
        onAccepted: {
            mergePendingSources = []
            for (let i = 0; i < selectedFiles.length; ++i)
                mergePendingSources.push(app.localPath(selectedFiles[i]))
            if (mergePendingSources.length === 0)
                return
            const p = activePane()
            if (p)
                mergePendingSources.unshift(p.path)
            mergeDestDialog.open()
        }
    }

    FileDialog {
        id: mergeDestDialog
        title: qsTr("Merge PDFs — save as")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PDF files (*.pdf)")]
        defaultSuffix: "pdf"
        onAccepted: {
            const p = activePane()
            if (p && selectedFile && mergePendingSources.length >= 2)
                p.mergeFrom(mergePendingSources, app.localPath(selectedFile))
            mergePendingSources = []
        }
    }

    FileDialog {
        id: exportAnnotDialog
        title: qsTr("Export annotations to PDF")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PDF files (*.pdf)")]
        defaultSuffix: "pdf"
        onAccepted: {
            const p = activePane()
            if (p && selectedFile)
                p.exportAnnotsTo(app.localPath(selectedFile))
        }
    }

    PasswordDialog {
        id: passwordDialog
        anchors.centerIn: parent
        onAcceptedPassword: (path, password) => {
            const p = activePane()
            if (p && p.path === path)
                p.setPassword(password)
        }
        onCancelled: (path) => {
            for (let i = 0; i < app.tabs.count; ++i) {
                if (app.tabPathAt(i) === path) {
                    app.closeTab(i)
                    win.statusText = qsTr("Unlock cancelled")
                    return
                }
            }
        }
    }

    LinkConfirmDialog {
        id: linkDialog
        anchors.centerIn: parent
        url: app.pendingLink
        visible: app.pendingLink.toString().length > 0
        onDecision: (ok) => app.confirmPendingLink(ok)
    }

    Connections {
        target: app
        function onOpenFailed(path, reason) { win.statusText = path + ": " + reason }
        function onStatusMessage(message) { win.statusText = message }
        function onDocumentFileChanged(path) {
            for (let i = 0; i < pdfRepeater.count; ++i) {
                const p = pdfRepeater.itemAt(i)
                if (!p || p.path !== path)
                    continue
                if (!p.enabled)
                    return
                if (p.structureAwaiting || structure.busy)
                    return
                if (p.dirty) {
                    win.statusText = qsTr("File changed on disk — save or discard annotations, then reload")
                    return
                }
                p.reloadFromDisk()
                win.statusText = qsTr("Reloaded (file changed on disk)")
                return
            }
        }
    }

    onClosing: (close) => {
        for (let i = 0; i < pdfRepeater.count; ++i) {
            const p = pdfRepeater.itemAt(i)
            if (p && p.dirty) {
                close.accepted = false
                closingWindow = true
                pendingCloseIndex = i
                dirtyDialog.fileLabel = app.tabTitleAt(i)
                dirtyDialog.open()
                return
            }
        }
        for (let i = 0; i < pdfRepeater.count; ++i) {
            const p = pdfRepeater.itemAt(i)
            if (p)
                p.persistState()
        }
    }
}

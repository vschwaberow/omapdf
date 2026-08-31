// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Pdf
import QtQuick.Shapes

/*!
    \qmltype PdfMultiPageView
    \inqmlmodule QtQuick.Pdf
    \brief A complete PDF viewer component for scrolling through multiple pages.

    PdfMultiPageView provides a PDF viewer component that offers a user
    experience similar to many common PDF viewer applications. It supports
    flicking through the pages in the entire document, with narrow gaps between
    the page images.

    PdfMultiPageView also supports selecting text and copying it to the
    clipboard, zooming in and out, clicking an internal link to jump to another
    section in the document, rotating the view, and searching for text. The
    \l {PDF Multipage Viewer Example} demonstrates how to use these features
    in an application.

    The implementation is a QML assembly of smaller building blocks that are
    available separately. In case you want to make changes in your own version
    of this component, you can copy the QML, which is installed into the
    \c QtQuick/Pdf/qml module directory, and modify it as needed.

    \sa PdfPageView, PdfScrollablePageView, PdfStyle
*/
Item {
    /*!
        \qmlproperty PdfDocument PdfMultiPageView::document

        A PdfDocument object with a valid \c source URL is required:

        \snippet multipageview.qml 0
    */
    required property PdfDocument document

    /*!
        \qmlproperty PdfDocument PdfMultiPageView::selectedText

        The selected text.
    */
    property string selectedText
    property var annotStore: null
    property int selectionPage: -1
    property var searchHitPages: ({})
    property var selectionGeometry: []
    property int lastTapPage: -1
    property real lastTapX: 0
    property real lastTapY: 0

    signal copySucceeded()

    function pageItem(pageIndex) {
        if (pageIndex < 0)
            return null
        return tableView.itemAtCell(Qt.point(0, pageIndex))
    }

    function captureSelection() {
        if (!root.selectedText.length || root.selectionPage < 0)
            return null
        if (!root.selectionGeometry || root.selectionGeometry.length === 0)
            return null
        return {
            page: root.selectionPage,
            text: root.selectedText,
            geometry: root.selectionGeometry
        }
    }

    function annotFill(hex) {
        const r = parseInt(hex.substring(1, 3), 16) / 255
        const g = parseInt(hex.substring(3, 5), 16) / 255
        const b = parseInt(hex.substring(5, 7), 16) / 255
        return Qt.rgba(r, g, b, 0.35)
    }

    /*!
        \qmlmethod void PdfMultiPageView::selectAll()

        Selects all the text on the \l {currentPage}{current page}, and makes it
        available as the system \l {QClipboard::Selection}{selection} on systems
        that support that feature.

        \sa copySelectionToClipboard()
    */
    function selectAll() {
        const item = pageItem(pageNavigator.currentPage)
            ?? tableView.itemAtCell(tableView.cellAtPos(root.width / 2, root.height / 2))
        const pdfSelection = item?.selection as PdfSelection
        pdfSelection?.selectAll()
    }

    /*!
        \qmlmethod void PdfMultiPageView::copySelectionToClipboard()

        Copies the selected text (if any) to the
        \l {QClipboard::Clipboard}{system clipboard}.

        \sa selectAll()
    */
    function copySelectionToClipboard() {
        const item = pageItem(root.selectionPage)
            ?? tableView.itemAtCell(tableView.cellAtPos(root.width / 2, root.height / 2))
        const pdfSelection = item?.selection as PdfSelection
        if (pdfSelection && pdfSelection.text && pdfSelection.text.length) {
            pdfSelection.copyToClipboard()
            root.copySucceeded()
            return true
        }
        if (root.selectedText.length) {
            app.copyText(root.selectedText)
            root.copySucceeded()
            return true
        }
        return false
    }

    // --------------------------------
    // page navigation

    /*!
        \qmlproperty int PdfMultiPageView::currentPage
        \readonly

        This property holds the zero-based page number of the page visible in the
        scrollable view. If there is no current page, it holds -1.

        This property is read-only, and is typically used in a binding (or
        \c onCurrentPageChanged script) to update the part of the user interface
        that shows the current page number, such as a \l SpinBox.

        \sa PdfPageNavigator::currentPage
    */
    property alias currentPage: pageNavigator.currentPage

    /*!
        \qmlproperty bool PdfMultiPageView::backEnabled
        \readonly

        This property indicates if it is possible to go back in the navigation
        history to a previous-viewed page.

        \sa PdfPageNavigator::backAvailable, back()
    */
    property alias backEnabled: pageNavigator.backAvailable

    /*!
        \qmlproperty bool PdfMultiPageView::forwardEnabled
        \readonly

        This property indicates if it is possible to go to next location in the
        navigation history.

        \sa PdfPageNavigator::forwardAvailable, forward()
    */
    property alias forwardEnabled: pageNavigator.forwardAvailable

    /*!
        \qmlmethod void PdfMultiPageView::back()

        Scrolls the view back to the previous page that the user visited most
        recently; or does nothing if there is no previous location on the
        navigation stack.

        \sa PdfPageNavigator::back(), currentPage, backEnabled
    */
    function back() { pageNavigator.back() }

    /*!
        \qmlmethod void PdfMultiPageView::forward()

        Scrolls the view to the page that the user was viewing when the back()
        method was called; or does nothing if there is no "next" location on the
        navigation stack.

        \sa PdfPageNavigator::forward(), currentPage
    */
    function forward() { pageNavigator.forward() }

    /*!
        \qmlmethod void PdfMultiPageView::goToPage(int page)

        Scrolls the view to the given \a page number, if possible.

        \sa PdfPageNavigator::jump(), currentPage
    */
    function goToPage(page) {
        if (page === pageNavigator.currentPage)
            return
        goToLocation(page, Qt.point(-1, -1), 0)
    }

    /*!
        \qmlmethod void PdfMultiPageView::goToLocation(int page, point location, real zoom)

        Scrolls the view to the \a location on the \a page, if possible,
        and sets the \a zoom level.

        \sa PdfPageNavigator::jump(), currentPage
    */
    function goToLocation(page, location, zoom) {
        if (tableView.rows === 0) {
            // save this request for later
            tableView.pendingRow = page
            tableView.pendingLocation = location
            tableView.pendingZoom = zoom
            return
        }
        if (zoom > 0) {
            pageNavigator.jumping = true // don't call pageNavigator.update() because we will jump() instead
            root.renderScale = zoom
            pageNavigator.jumping = false
        }
        pageNavigator.jump(page, location, zoom) // actually jump
    }

    /*!
        \qmlproperty int PdfMultiPageView::currentPageRenderingStatus

        This property holds the \l {QtQuick::Image::status}{rendering status} of
        the \l {currentPage}{current page}.
    */
    property int currentPageRenderingStatus: Image.Null

    // --------------------------------
    // page scaling

    /*!
        \qmlproperty real PdfMultiPageView::renderScale

        This property holds the ratio of pixels to points. The default is \c 1,
        meaning one point (1/72 of an inch) equals 1 logical pixel.
    */
    property real renderScale: 1
    readonly property alias contentY: tableView.contentY
    readonly property alias contentHeight: tableView.contentHeight
    function setContentY(y) { tableView.contentY = y }


    /*!
        \qmlproperty real PdfMultiPageView::pageRotation

        This property holds the clockwise rotation of the pages.

        The default value is \c 0 degrees (that is, no rotation relative to the
        orientation of the pages as stored in the PDF file).
    */
    property real pageRotation: 0

    /*!
        \qmlmethod void PdfMultiPageView::resetScale()

        Sets \l renderScale back to its default value of \c 1.
    */
    function resetScale() { root.renderScale = 1 }

    /*!
        \qmlmethod void PdfMultiPageView::scaleToWidth(real width, real height)

        Sets \l renderScale such that the width of the first page will fit into a
        viewport with the given \a width and \a height. If the page is not rotated,
        it will be scaled so that its width fits \a width. If it is rotated +/- 90
        degrees, it will be scaled so that its width fits \a height.
    */
    function scaleToWidth(width, height) {
        root.renderScale = width / (tableView.rot90 ? tableView.firstPagePointSize.height : tableView.firstPagePointSize.width)
    }

    /*!
        \qmlmethod void PdfMultiPageView::scaleToPage(real width, real height)

        Sets \l renderScale such that the whole first page will fit into a viewport
        with the given \a width and \a height. The resulting \l renderScale depends
        on \l pageRotation: the page will fit into the viewport at a larger size if
        it is first rotated to have a matching aspect ratio.
    */
    function scaleToPage(width, height) {
        const windowAspect = width / height
        const pageAspect = tableView.firstPagePointSize.width / tableView.firstPagePointSize.height
        if (tableView.rot90) {
            if (windowAspect > pageAspect) {
                root.renderScale = height / tableView.firstPagePointSize.width
            } else {
                root.renderScale = width / tableView.firstPagePointSize.height
            }
        } else {
            if (windowAspect > pageAspect) {
                root.renderScale = height / tableView.firstPagePointSize.height
            } else {
                root.renderScale = width / tableView.firstPagePointSize.width
            }
        }
    }

    // --------------------------------
    // text search

    /*!
        \qmlproperty PdfSearchModel PdfMultiPageView::searchModel

        This property holds a PdfSearchModel containing the list of search results
        for a given \l searchString.

        \sa PdfSearchModel
    */
    property alias searchModel: searchModel

    /*!
        \qmlproperty string PdfMultiPageView::searchString

        This property holds the search string that the user may choose to search
        for. It is typically used in a binding to the \c text property of a
        TextField.

        \sa searchModel
    */
    property alias searchString: searchModel.searchString

    function rebuildSearchHitPages() {
        const pages = {}
        const n = searchModel.count
        for (let i = 0; i < n; ++i) {
            const page = searchModel.data(searchModel.index(i, 0), 0x0100)
            if (page !== undefined && page !== null)
                pages[page] = true
        }
        root.searchHitPages = pages
    }

    /*!
        \qmlmethod void PdfMultiPageView::searchBack()

        Decrements the
        \l{PdfSearchModel::currentResult}{searchModel's current result}
        so that the view will jump to the previous search result.
    */
    function searchBack() { --searchModel.currentResult }

    /*!
        \qmlmethod void PdfMultiPageView::searchForward()

        Increments the
        \l{PdfSearchModel::currentResult}{searchModel's current result}
        so that the view will jump to the next search result.
    */
    function searchForward() { ++searchModel.currentResult }

    LoggingCategory {
        id: lcMPV
        name: "qt.pdf.multipageview"
    }

    id: root
    PdfStyle { id: style }
    TableView {
        id: tableView
        property bool debug: false
        property real minScale: 0.1
        property real maxScale: 10
        property point jumpLocationMargin: Qt.point(10, 10)  // px away from viewport edges
        anchors.fill: parent
        anchors.leftMargin: 2
        reuseItems: true
        model: root.document ? root.document.pageCount : 0
        rowSpacing: 6
        property bool sharpRender: true
        property real rotationNorm: Math.round((360 + (root.pageRotation % 360)) % 360)
        property bool rot90: rotationNorm == 90 || rotationNorm == 270
        onRot90Changed: forceLayout()
        onHeightChanged: layoutDebounce.restart()
        onWidthChanged: layoutDebounce.restart()
        onMovingChanged: {
            if (moving) {
                idleSharpen.stop()
                sharpRender = false
            } else {
                idleSharpen.restart()
            }
        }

        Timer {
            id: layoutDebounce
            interval: 16
            onTriggered: tableView.forceLayout()
        }
        Timer {
            id: idleSharpen
            interval: 150
            onTriggered: tableView.sharpRender = true
        }
        property size firstPagePointSize: root.document?.status === PdfDocument.Ready ? root.document.pagePointSize(0) : Qt.size(1, 1)
        property real pageHolderWidth: Math.max(root.width, ((rot90 ? root.document?.maxPageHeight : root.document?.maxPageWidth) ?? 0) * root.renderScale)
        columnWidthProvider: function(col) { return root.document ? pageHolderWidth + vscroll.width + 2 : 0 }
        rowHeightProvider: function(row) { return (rot90 ? root.document.pagePointSize(row).width : root.document.pagePointSize(row).height) * root.renderScale }

        // delayed-jump feature in case the user called goToPage() or goToLocation() too early
        property int pendingRow: -1
        property point pendingLocation
        property real pendingZoom: -1
        onRowsChanged: {
            if (rows > 0 && tableView.pendingRow >= 0) {
                if (tableView.debug)
                    console.log(lcMPV, "initiating delayed jump to page", tableView.pendingRow, "loc", tableView.pendingLocation, "zoom", tableView.pendingZoom)
                root.goToLocation(tableView.pendingRow, tableView.pendingLocation, tableView.pendingZoom)
                tableView.pendingRow = -1
                tableView.pendingLocation = Qt.point(-1, -1)
                tableView.pendingZoom = -1
            }
        }

        delegate: Rectangle {
            id: pageHolder
            required property int index
            color: tableView.debug ? "beige" : "transparent"
            Text {
                visible: tableView.debug
                anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                rotation: -90; text: pageHolder.width.toFixed(1) + "x" + pageHolder.height.toFixed(1) + "\n" +
                                     image.width.toFixed(1) + "x" + image.height.toFixed(1)
            }
            property alias selection: selection
            Rectangle {
                id: paper
                width: image.width
                height: image.height
                rotation: root.pageRotation
                anchors.centerIn: pinch.active ? undefined : parent
                property size pagePointSize: root.document.pagePointSize(pageHolder.index)
                property real pageScale: image.paintedWidth / pagePointSize.width
                HoverHandler {
                    cursorShape: Qt.IBeamCursor
                }
                PdfPageImage {
                    id: image
                    document: root.document
                    currentFrame: pageHolder.index
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    width: paper.pagePointSize.width * root.renderScale
                    height: paper.pagePointSize.height * root.renderScale
                    property real renderScale: root.renderScale
                    property real oldRenderScale: 1
                    function applySourceSize() {
                        const dpr = tableView.sharpRender ? Screen.devicePixelRatio : 1.0
                        let w = paper.pagePointSize.width * renderScale * dpr
                        const maxEdge = 4096
                        if (w > maxEdge)
                            w = maxEdge
                        image.sourceSize.width = w
                        image.sourceSize.height = 0
                    }
                    onRenderScaleChanged: {
                        applySourceSize()
                        paper.scale = 1
                        if (!pinch.active && !tableView.moving)
                            searchHighlights.update()
                    }
                    Connections {
                        target: tableView
                        function onSharpRenderChanged() { image.applySourceSize() }
                    }
                    onStatusChanged: {
                        if (pageHolder.index === pageNavigator.currentPage)
                            root.currentPageRenderingStatus = status
                    }
                }
                Shape {
                    anchors.fill: parent
                    visible: image.status === Image.Ready
                    ShapePath {
                        strokeWidth: -1
                        fillColor: style.selectionColor
                        scale: Qt.size(paper.pageScale, paper.pageScale)
                        PathMultiline {
                            paths: selection.geometry
                        }
                    }
                }
                Shape {
                    anchors.fill: parent
                    visible: image.status === Image.Ready
                             && searchModel.searchString.length > 0
                             && !tableView.moving
                             && root.searchHitPages[pageHolder.index] === true
                    onVisibleChanged: searchHighlights.update()
                    ShapePath {
                        strokeWidth: -1
                        fillColor: style.pageSearchResultsColor
                        scale: Qt.size(paper.pageScale, paper.pageScale)
                        PathMultiline {
                            id: searchHighlights
                            function update() {
                                // paths could be a binding, but we need to be able to "kick" it sometimes
                                if (!searchModel.searchString.length || tableView.moving
                                        || root.searchHitPages[pageHolder.index] !== true) {
                                    paths = []
                                    return
                                }
                                paths = searchModel.boundingPolygonsOnPage(pageHolder.index)
                            }
                        }
                    }
                    Connections {
                        target: searchModel
                        // whenever the highlights on the _current_ page change, they actually need to change on _all_ pages
                        // (usually because the search string has changed)
                        function onCurrentPageBoundingPolygonsChanged() { searchHighlights.update() }
                    }
                }
                Item {
                    id: annotLayer
                    anchors.fill: parent
                    visible: image.status === Image.Ready && root.annotStore !== null && !tableView.moving
                    property int kick: 0
                    property var entries: {
                        kick
                        return root.annotStore ? root.annotStore.polygonsOnPage(pageHolder.index) : []
                    }
                    Repeater {
                        model: annotLayer.entries
                        Shape {
                            anchors.fill: parent
                            required property var modelData
                            ShapePath {
                                strokeWidth: -1
                                fillColor: root.annotFill(modelData.color)
                                scale: Qt.size(paper.pageScale, paper.pageScale)
                                PathMultiline {
                                    paths: modelData.paths
                                }
                            }
                        }
                    }
                    Connections {
                        target: root.annotStore
                        function onRowsInserted() { ++annotLayer.kick }
                        function onRowsRemoved() { ++annotLayer.kick }
                        function onModelReset() { ++annotLayer.kick }
                        function onDataChanged() { ++annotLayer.kick }
                    }
                }
                Repeater {
                    model: {
                        if (tableView.moving || !root.annotStore)
                            return []
                        annotLayer.kick
                        return root.annotStore.notesOnPage(pageHolder.index)
                    }
                    delegate: Rectangle {
                        required property var modelData
                        width: 14
                        height: 14
                        radius: 2
                        color: modelData.color
                        x: modelData.x * paper.pageScale - width / 2
                        y: modelData.y * paper.pageScale - height / 2
                        z: 5
                        ToolTip.visible: noteHover.hovered
                        ToolTip.text: modelData.text
                        HoverHandler { id: noteHover }
                    }
                }
                Shape {
                    anchors.fill: parent
                    visible: image.status === Image.Ready
                             && !tableView.moving
                             && searchModel.currentPage === pageHolder.index
                    ShapePath {
                        strokeWidth: style.currentSearchResultStrokeWidth
                        strokeColor: style.currentSearchResultStrokeColor
                        fillColor: "transparent"
                        scale: Qt.size(paper.pageScale, paper.pageScale)
                        PathMultiline {
                            paths: searchModel.currentResultBoundingPolygons
                        }
                    }
                }
                PinchHandler {
                    id: pinch
                    minimumScale: tableView.minScale / root.renderScale
                    maximumScale: Math.max(1, tableView.maxScale / root.renderScale)
                    minimumRotation: root.pageRotation
                    maximumRotation: root.pageRotation
                    onActiveChanged:
                        if (active) {
                            paper.z = 10
                        } else {
                            paper.z = 0
                            const centroidInPoints = Qt.point(pinch.centroid.position.x / root.renderScale,
                                                            pinch.centroid.position.y / root.renderScale)
                            const centroidInFlickable = tableView.mapFromItem(paper, pinch.centroid.position.x, pinch.centroid.position.y)
                            const newSourceWidth = image.sourceSize.width * paper.scale
                            const ratio = newSourceWidth / image.sourceSize.width
                            if (tableView.debug)
                                console.log(lcMPV, "pinch ended on page", pageHolder.index,
                                "with scale", paper.scale.toFixed(3), "ratio", ratio.toFixed(3),
                                "centroid", pinch.centroid.position, centroidInPoints,
                                "wrt flickable", centroidInFlickable,
                                "page at", pageHolder.x.toFixed(2), pageHolder.y.toFixed(2),
                                "contentX/Y were", tableView.contentX.toFixed(2), tableView.contentY.toFixed(2))
                            if (ratio > 1.1 || ratio < 0.9) {
                                const centroidOnPage = Qt.point(centroidInPoints.x * root.renderScale * ratio, centroidInPoints.y * root.renderScale * ratio)
                                paper.scale = 1
                                pinch.persistentScale = 1
                                paper.x = 0
                                paper.y = 0
                                root.renderScale *= ratio
                                if (tableView.rotationNorm == 0) {
                                    tableView.contentX = pageHolder.x + tableView.originX + centroidOnPage.x - centroidInFlickable.x
                                    tableView.contentY = pageHolder.y + tableView.originY + centroidOnPage.y - centroidInFlickable.y
                                } else if (tableView.rotationNorm == 90) {
                                    tableView.contentX = pageHolder.x + tableView.originX + image.height - centroidOnPage.y - centroidInFlickable.x
                                    tableView.contentY = pageHolder.y + tableView.originY + centroidOnPage.x - centroidInFlickable.y
                                } else if (tableView.rotationNorm == 180) {
                                    tableView.contentX = pageHolder.x + tableView.originX + image.width - centroidOnPage.x - centroidInFlickable.x
                                    tableView.contentY = pageHolder.y + tableView.originY + image.height - centroidOnPage.y - centroidInFlickable.y
                                } else if (tableView.rotationNorm == 270) {
                                    tableView.contentX = pageHolder.x + tableView.originX + centroidOnPage.y - centroidInFlickable.x
                                    tableView.contentY = pageHolder.y + tableView.originY + image.width - centroidOnPage.x - centroidInFlickable.y
                                }
                                if (tableView.debug)
                                    console.log(lcMPV, "contentX/Y adjusted to", tableView.contentX.toFixed(2), tableView.contentY.toFixed(2), "y @top", pageHolder.y)
                                tableView.returnToBounds()
                            }
                        }
                    grabPermissions: PointerHandler.CanTakeOverFromAnything
                }
                DragHandler {
                    id: textSelectionDrag
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus
                    target: null
                    onActiveChanged: {
                        if (active)
                            return
                        if (selection.text.length)
                            app.copyTextToSelection(selection.text)
                    }
                }
                TapHandler {
                    id: mouseClickHandler
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus
                    onTapped: (eventPoint) => {
                        root.lastTapPage = pageHolder.index
                        root.lastTapX = eventPoint.position.x / paper.pageScale
                        root.lastTapY = eventPoint.position.y / paper.pageScale
                    }
                }
                TapHandler {
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus
                    acceptedButtons: Qt.RightButton
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    onTapped: (eventPoint) => {
                        selectionMenu.popup(paper, eventPoint.position)
                    }
                }
                TapHandler {
                    id: touchTapHandler
                    acceptedDevices: PointerDevice.TouchScreen
                    onTapped: (eventPoint) => {
                        root.lastTapPage = pageHolder.index
                        root.lastTapX = eventPoint.position.x / paper.pageScale
                        root.lastTapY = eventPoint.position.y / paper.pageScale
                        selection.clear()
                        selection.forceActiveFocus()
                    }
                }
                Repeater {
                    model: PdfLinkModel {
                        id: linkModel
                        document: root.document
                        page: image.currentFrame
                    }
                    delegate: PdfLinkDelegate {
                        x: rectangle.x * paper.pageScale
                        y: rectangle.y * paper.pageScale
                        width: rectangle.width * paper.pageScale
                        height: rectangle.height * paper.pageScale
                        visible: image.status === Image.Ready
                        onTapped:
                            (link) => {
                                if (link.page >= 0)
                                    root.goToLocation(link.page, link.location, link.zoom)
                                else
                                    Qt.openUrlExternally(url)
                            }
                    }
                }
                PdfSelection {
                    id: selection
                    anchors.fill: parent
                    document: root.document
                    page: image.currentFrame
                    renderScale: image.renderScale
                    from: textSelectionDrag.centroid.pressPosition
                    to: textSelectionDrag.centroid.position
                    hold: !textSelectionDrag.active && !mouseClickHandler.pressed
                    onTextChanged: {
                        root.selectedText = text
                        root.selectionPage = page
                        root.selectionGeometry = geometry
                        if (text.length && !textSelectionDrag.active)
                            app.copyTextToSelection(text)
                    }
                    focus: true
                }
            }
        }
        ScrollBar.vertical: ScrollBar {
            id: vscroll
            property bool moved: false
            onPositionChanged: moved = true
            onPressedChanged: if (pressed) {
                // When the user starts scrolling, push the location where we came from so the user can go "back" there
                const cell = tableView.cellAtPos(root.width / 2, root.height / 2)
                const currentItem = tableView.itemAtCell(cell)
                const currentLocation = currentItem
                                      ? Qt.point((tableView.contentX - currentItem.x + tableView.jumpLocationMargin.x) / root.renderScale,
                                                 (tableView.contentY - currentItem.y + tableView.jumpLocationMargin.y) / root.renderScale)
                                      : Qt.point(0, 0) // maybe the delegate wasn't loaded yet
                pageNavigator.jump(cell.y, currentLocation, root.renderScale)
            }
            onActiveChanged: if (!active ) {
                // When the scrollbar stops moving, tell navstack where we are, so as to update currentPage etc.
                const cell = tableView.cellAtPos(root.width / 2, root.height / 2)
                const currentItem = tableView.itemAtCell(cell)
                const currentLocation = currentItem
                                      ? Qt.point((tableView.contentX - currentItem.x + tableView.jumpLocationMargin.x) / root.renderScale,
                                                 (tableView.contentY - currentItem.y + tableView.jumpLocationMargin.y) / root.renderScale)
                                      : Qt.point(0, 0) // maybe the delegate wasn't loaded yet
                pageNavigator.update(cell.y, currentLocation, root.renderScale)
            }
        }
        ScrollBar.horizontal: ScrollBar { }
    }
    onRenderScaleChanged: {
        // if pageNavigator.jumped changes the scale, don't turn around and update the stack again;
        // and don't force layout either, because positionViewAtCell() will do that
        if (pageNavigator.jumping)
            return
        // page size changed: TableView needs to redo layout to avoid overlapping delegates or gaps between them
        layoutDebounce.restart()
        const cell = tableView.cellAtPos(root.width / 2, root.height / 2)
        const currentItem = tableView.itemAtCell(cell)
        if (currentItem) {
            const currentLocation = Qt.point((tableView.contentX - currentItem.x + tableView.jumpLocationMargin.x) / root.renderScale,
                                             (tableView.contentY - currentItem.y + tableView.jumpLocationMargin.y) / root.renderScale)
            pageNavigator.update(cell.y, currentLocation, renderScale)
        }
    }
    PdfPageNavigator {
        id: pageNavigator
        property bool jumping: false
        property int previousPage: 0
        onJumped: function(current) {
            jumping = true
            if (current.zoom > 0)
                root.renderScale = current.zoom
            const pageSize = root.document.pagePointSize(current.page)
            if (current.location.y < 0) {
                // invalid to indicate that a specific location was not needed,
                // so attempt to position the new page just as the current page is
                const previousPageDelegate = tableView.itemAtCell(0, previousPage)
                const currentYOffset = previousPageDelegate
                                     ? tableView.contentY - previousPageDelegate.y
                                     : 0
                tableView.positionViewAtRow(current.page, Qt.AlignTop, currentYOffset)
                if (tableView.debug)
                    console.log(lcMPV, "going from page", previousPage, "to", current.page, "offset", currentYOffset,
                    "ended up @", tableView.contentX.toFixed(1) + ", " + tableView.contentY.toFixed(1))
            } else if (current.rectangles.length > 0) {
                // jump to a search result and position the covered area within the viewport
                pageSize.width *= root.renderScale
                pageSize.height *= root.renderScale
                const rectPts = current.rectangles[0]
                const rectPx = Qt.rect(rectPts.x * root.renderScale - tableView.jumpLocationMargin.x,
                                       rectPts.y * root.renderScale - tableView.jumpLocationMargin.y,
                                       rectPts.width * root.renderScale + tableView.jumpLocationMargin.x * 2,
                                       rectPts.height * root.renderScale + tableView.jumpLocationMargin.y * 2)
                tableView.positionViewAtCell(0, current.page, TableView.Contain, Qt.point(0, 0), rectPx)
                if (tableView.debug)
                    console.log(lcMPV, "going to zoom", root.renderScale, "rect", rectPx, "on page", current.page,
                    "ended up @", tableView.contentX.toFixed(1) + ", " + tableView.contentY.toFixed(1))
            } else {
                // jump to a page and position the given location relative to the top-left corner of the viewport
                pageSize.width *= root.renderScale
                pageSize.height *= root.renderScale
                const rectPx = Qt.rect(current.location.x * root.renderScale - tableView.jumpLocationMargin.x,
                                       current.location.y * root.renderScale - tableView.jumpLocationMargin.y,
                                       tableView.jumpLocationMargin.x * 2, tableView.jumpLocationMargin.y * 2)
                tableView.positionViewAtCell(0, current.page, TableView.AlignLeft | TableView.AlignTop, Qt.point(0, 0), rectPx)
                if (tableView.debug)
                    console.log(lcMPV, "going to zoom", root.renderScale, "loc", current.location, "on page", current.page,
                    "ended up @", tableView.contentX.toFixed(1) + ", " + tableView.contentY.toFixed(1))
            }
            jumping = false
            previousPage = current.page
        }

        property url documentSource: root.document.source
        onDocumentSourceChanged: {
            pageNavigator.clear()
            root.resetScale()
            tableView.contentX = 0
            tableView.contentY = 0
        }
    }
    PdfSearchModel {
        id: searchModel
        document: root.document === undefined ? null : root.document
        onCurrentResultChanged: pageNavigator.jump(currentResultLink)
        onCountChanged: root.rebuildSearchHitPages()
        onSearchStringChanged: root.rebuildSearchHitPages()
    }

    Menu {
        id: selectionMenu
        MenuItem {
            text: qsTr("Copy")
            enabled: root.selectedText.length > 0
            onTriggered: root.copySelectionToClipboard()
        }
    }
}

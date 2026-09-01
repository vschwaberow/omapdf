# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed

- Acrobat-style selection: word (double-click), line (triple-click), shift-extend, click-to-clear, edge auto-scroll
- Cross-page text selection while dragging or shift-extending across page breaks
- Word-snap while dragging; continuous edge auto-scroll; multi-page highlight from one selection
- Move text selection domain into C++ (`PdfTextSource` + `TextSelectionModel`); QML keeps gestures only
- Selection highlight only on the active page
- Mouse text selection no longer loses the grab to pinch / flick; selection highlight stays during drag
- PdfSelection maps with page index and renderScale from the page holder

### Changed

- Pinch zoom accepts touch only so mouse/stylus can select text

### Added

- Tool rail buttons for copy selection, select-all (page), and highlight

## [0.2.2] - 2026-09-01

### Fixed

- Restore Qt `PdfPageImage` for page display (drop custom `PageTileLayer` from the viewer and binary)
- RAII connections and QPointer guards around PDF warmup document bindings
- Shutdown no longer segfaults when `QPdfDocument` closes under `PageWarmup`
- Disk/structure reload restores `contentY` with zoom and page
- Reading gate measures full-page rasters matching the viewer sourceSize contract
- Harden commit-msg stripping for `Made with [Cursor](...)` markdown footers

### Changed

- Soft zoom: layout follows immediately; PdfPageImage sourceSize sharpens after 120 ms idle; wheel debounce 80 ms
- Remove unused page-tile hub/layer sources from the shipped app

### Added

- `scripts/verify-l1-beauty.sh` and `scripts/verify-demo-ready.sh`
- `docs/demo-script.md` launch recording outline
- Reading gate cold first-page ink budget (500 ms)
- `viewer_page_image` and `l1_beauty` ctests

## [0.2.1] - 2026-08-31

### Fixed

- Fix startup: avoid `PdfSearchModel.onCountChanged` (not exposed in QtQuick.Pdf 6.8)

## [0.2.0] - 2026-08-31

### Changed

- Reading gate: visible four-tile viewport render budget
- Directional tile prefetch; hub epoch cancel; viewport-center request order
- SceneGraph textures for page tiles; adaptive 256/512/1024 tile size
- Low-res page placeholder under sharp viewport tiles
- Soft tile rescale: keep prior tiles until replacements arrive
- Cap thumbnail raster width at 256 px
- Reuse the native `QPdfDocument` inside QML `PdfDocument` (no per-layer reload)
- Share one `QPdfPageRenderer` across page tile layers via `PageTileHub`
- Debounce tool-rail zoom percent; sharpen after 100 ms idle
- Pause page-tile requests while flicking; detach link model during move
- Viewport tile cache: render visible page clips via `PageTileLayer`
- Faster flick scrolling: lower page raster DPI while moving, sharpen after idle
- Hide search and annot overlays during scroll; skip search shapes on pages without hits
- Warm adjacent pages via `PageWarmup` (`QPdfPageRenderer`)
- Cache annot polygons/notes per page in `AnnotStore`
- Loading veil on tab open until the PDF is ready; coalesce pinch/scale layouts
- Reading gate covers idle-sharpen render windows
- Debounce Ctrl+Wheel zoom; cap page texture edge at 4096 px
- Skip note markers and current-search stroke while scrolling
- Warm ±2 pages; debounce warmup tile size on resize
- Pause page warmup while flicking; hide link hit-targets while moving
- Selection shape only when text is selected; debounce thumbnail follow
- Cache page extents for TableView row heights; coarser scroll-time DPR
- Debounce status-line updates while scrolling; hide selection while moving

## [0.1.0] - 2026-08-31

First release.

### Added

- Qt 6 Quick PDF reader with continuous vertical scroll (`OmapdfMultiPageView`)
- In-app tabs, as-you-type search, and live Omarchy theme from TOML
- Reader parity: copy, select all, fit width/page, print, password prompt, external link confirm
- Resizable left chrome (thumbnails, outline, reader) via SplitView
- Sidecar highlights and notes under `~/.local/state/omapdf/`
- Optional power features: libqpdf page ops (rotate, delete, extract, merge) and annot export
- Per-document session restore (zoom, page, scroll, dim)
- DE/EN locale support
- CTest suite including `reading_gate` performance budgets
- Desktop entry, icon, and `docs/usage.md` keyboard reference
- Version `0.1.0` from CMake (`--version`, welcome screen, window title)

### Fixed

- Startup failure on Qt 6.11: `TableView` has no `cacheBuffer` property

[Unreleased]: https://github.com/vschwaberow/omapdf/compare/v0.2.2...HEAD
[0.2.2]: https://github.com/vschwaberow/omapdf/releases/tag/v0.2.2
[0.2.1]: https://github.com/vschwaberow/omapdf/releases/tag/v0.2.1
[0.2.0]: https://github.com/vschwaberow/omapdf/releases/tag/v0.2.0
[0.1.0]: https://github.com/vschwaberow/omapdf/releases/tag/v0.1.0

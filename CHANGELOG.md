# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

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

[0.1.0]: https://github.com/vschwaberow/omapdf/releases/tag/v0.1.0

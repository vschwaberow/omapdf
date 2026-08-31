<p align="center">
  <img src="assets/omapdf-logo.svg" alt="omapdf" width="560">
</p>

**The fastest, most beautiful PDF reading engine for Linux.**

Native Qt 6 Quick reader for [Omarchy](https://omarchy.org) (Hyprland). Continuous vertical scroll with viewport tiles, as-you-type search, live Omarchy theme, tabs, and classic reader parity — under MIT.

**v1 promise:** reading speed and chrome beauty.  
**Not a v1 claim:** editing, forms, presentation mode. Highlight/notes and page-structure tools ship in the binary as optional power features; see [docs/usage.md](docs/usage.md).

## Requirements

- Linux (Omarchy / Arch-like)
- CMake ≥ 3.21, Ninja, GCC with C++26
- Qt ≥ 6.8 (`Qt6::{Core,Gui,Quick,QuickControls2,Pdf,Widgets,PrintSupport,Concurrent,Qml}`)
- `libqpdf` (Apache-2.0) for power structure ops — not Poppler

## Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/omapdf [files...]
```

Several files open as tabs in one window. Each new process is a new window.

### Debug and tests

```bash
cmake -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
ctest --test-dir build-debug --output-on-failure
```

Debug builds enable ASan/UBSan when configured that way in CMake. Clang-tidy:

```bash
cmake -B build-tidy -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOMAPDF_CLANG_TIDY=ON
cmake --build build-tidy
```

`./build/omapdf --verbose …` keeps Qt `qt.pdf.links` warnings visible; they are muted by default.

## Install

```bash
cmake --install build --prefix ~/.local
update-desktop-database ~/.local/share/applications
gtk-update-icon-cache -f ~/.local/share/icons/hicolor 2>/dev/null || true
```

That installs the binary, `omapdf.desktop`, and the scalable icon (MIME `application/pdf`).

### Default PDF handler

```bash
./scripts/set-default-pdf-handler.sh
```

Or: `xdg-mime default omapdf.desktop application/pdf`

## What you get (v1)

| Area | Behavior |
|------|----------|
| Scroll | Continuous multi-page scroll; viewport tiles + idle sharpen |
| Search | `/` or Ctrl+F, as-you-type; `n` / `N` (F3 / Shift+F3) |
| Theme | Live Omarchy `colors.toml` / `shell.toml` |
| Tabs | In-app tabs; outline when the PDF has bookmarks |
| Parity | Copy, link confirm, fit width/page, print (system dialog), password prompt |
| Keys | Vim-style and classic bindings in parallel |
| Empty | Welcome, recents, drag-and-drop; `o` / Ctrl+O |

Per-document state (zoom, page, scroll, dim) lives under `~/.local/state/omapdf/`.

Keyboard reference and power features: **[docs/usage.md](docs/usage.md)**.

## Version

Current release: **0.2.1**. See [CHANGELOG.md](CHANGELOG.md).

## License

MIT — see [LICENSE](LICENSE).

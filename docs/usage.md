# Usage

omapdf is a reading engine first. Shortcuts below are active when the search field is not focused, unless noted.

## Open and window

| Action | Keys |
|--------|------|
| Open file(s) | `o`, Ctrl+O |
| Close tab | Ctrl+W |
| Quit | Ctrl+Q |
| Next / previous tab | Ctrl+Tab / Ctrl+Shift+Tab |
| Reload file | F5 / Refresh |
| Middle-click tab | Close that tab |

CLI: `omapdf a.pdf b.pdf` → one window, two tabs. A new process is always a new window.

## Navigate

| Action | Keys |
|--------|------|
| Page down / up | `j` / `k`, PgDown / PgUp |
| Viewport ~90% | Space / Shift+Space |
| First / last page | `g` / `G`, Home / End |
| History back / forward | Alt+Left / Alt+Right, mouse back / forward |
| Outline (bookmarks) | Ctrl+B (auto-opens when bookmarks exist) |
| Thumbnails | `t` (off by default) |

## Zoom and view

| Action | Keys |
|--------|------|
| Zoom in / out | `+` / `-`, Ctrl+= / Ctrl+- |
| Zoom with wheel | Ctrl+mouse wheel |
| Fit width | `w` |
| Fit page | `0` |
| Dim pages | `d` (per document) |

Fit width/page stays live with window resize until you zoom manually. Zoom, page, scroll offset, and dim are restored per file.

Pages render as viewport tiles (SceneGraph): low-res placeholder first, then sharp clips; coarser DPI while flicking, sharpen after idle. Ctrl+wheel zoom is debounced.

## Search

| Action | Keys |
|--------|------|
| Find | `/`, Ctrl+F |
| Next / previous hit | `n` / `N`, F3 / Shift+F3 |
| Clear / leave field | Escape (clears the query) |

Hit counter shows `i/n` in the search bar. No hit-list rail.

## Reader parity

| Action | Keys |
|--------|------|
| Copy selection | Ctrl+C, right-click Copy |
| Select all (page text) | Ctrl+A |
| Print | Ctrl+P (system dialog) |
| External link | Confirm dialog, then open |

Selecting text also fills the Linux primary selection (middle-click paste).

Password-protected PDFs prompt on open; cancel closes the tab. Wrong password retries with an error.

Locale follows the system (DE + EN; English fallback).

## Power features (not the v1 pitch)

Sidecar annotations and structure ops are available but not marketed as the v1 product. Annots live under `~/.local/state/omapdf/annots/` (not written into the PDF until export). Structure writes go through libqpdf and reload the view.

### Annotations

| Action | Keys |
|--------|------|
| Highlight selection | `h` |
| Add note | `a` |
| Annot color 1–4 | `1` `2` `3` `4` |
| Save annots | Ctrl+S |
| Undo / redo | Ctrl+Z / Ctrl+Y (Ctrl+Shift+Z) |
| Export annots into PDF | Ctrl+Shift+S |

Dirty annots block F5 reload and structure reloads until you save or discard.

### Structure

| Action | Keys |
|--------|------|
| Rotate page ±90° | `r` / Shift+R |
| Delete page | `x` (confirm) |
| Move page earlier / later | `[` / `]` |
| Extract page as PDF | Ctrl+E |
| Merge PDFs | Ctrl+Shift+M |

Shortcuts that mutate structure are disabled while an async structure/export job is busy.

## State paths

| Path | Contents |
|------|----------|
| `~/.local/state/omapdf/` | Session state, prefs, recents |
| `~/.local/state/omapdf/annots/` | Sidecar annotation JSON |

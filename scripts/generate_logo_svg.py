#!/usr/bin/env python3
"""Generate omapdf wordmark from Omarchy's FIGlet font and vectorize to SVG.

Omarchy ships /usr/share/omarchy/logo.txt in the Delta Corps Priest 1 style
(half-blocks). This script uses the same pyfiglet font for omapdf and emits
crisp axis-aligned rects (full and half cells).
"""

from __future__ import annotations

from pathlib import Path

import pyfiglet

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
FONT = "delta_corps_priest_1"
CELL_W = 12
CELL_H = 24
FILL = "#7aa2f7"


def ansi_art(text: str = "omapdf") -> str:
    art = pyfiglet.figlet_format(text, font=FONT)
    lines = [ln.rstrip("\n") for ln in art.splitlines()]
    while lines and not lines[-1].strip():
        lines.pop()
    while lines and not lines[0].strip():
        lines.pop(0)
    width = max((len(ln) for ln in lines), default=0)
    return "\n".join(ln.ljust(width) for ln in lines) + "\n"


def cell_frags(ch: str) -> list[tuple[float, float, float, float]]:
    if ch in (" ", ""):
        return []
    if ch == "\u2588":  # full block
        return [(0.0, 0.0, 1.0, 1.0)]
    if ch == "\u2584":  # lower half
        return [(0.0, 0.5, 1.0, 0.5)]
    if ch == "\u2580":  # upper half
        return [(0.0, 0.0, 1.0, 0.5)]
    if ch == "\u258c":  # left half
        return [(0.0, 0.0, 0.5, 1.0)]
    if ch == "\u2590":  # right half
        return [(0.5, 0.0, 0.5, 1.0)]
    if ch == "\u2596":
        return [(0.0, 0.5, 0.5, 0.5)]
    if ch == "\u2597":
        return [(0.5, 0.5, 0.5, 0.5)]
    if ch == "\u2598":
        return [(0.0, 0.0, 0.5, 0.5)]
    if ch == "\u259d":
        return [(0.5, 0.0, 0.5, 0.5)]
    if ch == "\u2599":
        return [(0.0, 0.0, 0.5, 1.0), (0.5, 0.5, 0.5, 0.5)]
    if ch == "\u259b":
        return [(0.0, 0.0, 1.0, 0.5), (0.0, 0.5, 0.5, 0.5)]
    if ch == "\u259c":
        return [(0.0, 0.0, 1.0, 0.5), (0.5, 0.5, 0.5, 0.5)]
    if ch == "\u259f":
        return [(0.5, 0.0, 0.5, 1.0), (0.0, 0.5, 0.5, 0.5)]
    if ch == "\u259e":
        return [(0.5, 0.0, 0.5, 0.5), (0.0, 0.5, 0.5, 0.5)]
    if ch == "\u259a":
        return [(0.0, 0.0, 0.5, 0.5), (0.5, 0.5, 0.5, 0.5)]
    return [(0.0, 0.0, 1.0, 1.0)]


def rects_from_art(art: str) -> tuple[list[tuple[float, float, float, float]], int, int]:
    rows = art.rstrip("\n").split("\n")
    cols = max(len(r) for r in rows)
    rows = [r.ljust(cols) for r in rows]
    out: list[tuple[float, float, float, float]] = []
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            for fx, fy, fw, fh in cell_frags(ch):
                out.append(
                    (
                        (x + fx) * CELL_W,
                        (y + fy) * CELL_H,
                        fw * CELL_W,
                        fh * CELL_H,
                    )
                )
    return out, cols, len(rows)


def to_svg(art: str) -> tuple[str, int, int]:
    rects, cols, rows = rects_from_art(art)
    vb_w = cols * CELL_W
    vb_h = rows * CELL_H
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {vb_w} {vb_h}" '
        f'width="100%" height="100%" role="img" aria-label="omapdf">',
        "  <title>omapdf</title>",
        f"  <!-- Omarchy-style FIGlet font={FONT}; cell={CELL_W}x{CELL_H} -->",
        f'  <g fill="{FILL}">',
    ]
    for x, y, w, h in rects:
        xs = int(x) if float(x).is_integer() else x
        ys = int(y) if float(y).is_integer() else y
        ws = int(w) if float(w).is_integer() else w
        hs = int(h) if float(h).is_integer() else h
        parts.append(f'    <rect x="{xs}" y="{ys}" width="{ws}" height="{hs}"/>')
    parts.append("  </g>")
    parts.append("</svg>\n")
    return "\n".join(parts), vb_w, vb_h


def main() -> None:
    art = ansi_art("omapdf")
    ansi_path = ASSETS / "omapdf-logo.ansi"
    svg_path = ASSETS / "omapdf-logo.svg"
    ansi_path.write_text(art)
    svg, vb_w, vb_h = to_svg(art)
    svg_path.write_text(svg)
    print(f"font={FONT}")
    print(f"ansi -> {ansi_path}")
    print(f"svg  -> {svg_path} viewBox={vb_w}x{vb_h} aspect={vb_h / vb_w:.6f}")


if __name__ == "__main__":
    main()

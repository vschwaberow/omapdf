# Demo script (20–30s)

Record on Omarchy with Screenkey. Goal: earn the tagline in one take.

| Time | Action | Show |
|------|--------|------|
| 0:00–0:04 | Neovim `paper.typ` left, omapdf right; `:w` | Live reload keeps zoom and scroll (no white flash) |
| 0:04–0:09 | Flick-scroll a ~200pp PDF; Ctrl+wheel zoom | Continuous `PdfPageImage` scroll, crisp DPR |
| 0:09–0:15 | Terminal: `omarchy theme set …` | Live chrome colors via ThemeBridge |
| 0:15–0:20 | `/` search, `n`/`N` | As-you-type search, no modal |
| 0:20–0:25 | Optional: open second PDF as tab | Tabs + Omarchy chrome |

End card: *Built for Omarchy. Qt 6 / PdfPageImage. Pure native craft.*

Verify before recording:

```bash
./scripts/verify-demo-ready.sh
./scripts/set-default-pdf-handler.sh   # optional MIME default
```

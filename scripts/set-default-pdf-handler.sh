#!/usr/bin/env bash
set -euo pipefail
desktop="${XDG_DATA_HOME:-$HOME/.local/share}/applications/omapdf.desktop"
if [[ ! -f "$desktop" ]]; then
  echo "missing $desktop — run: cmake --install build --prefix ~/.local" >&2
  exit 1
fi
if ! command -v xdg-mime >/dev/null; then
  echo "xdg-mime not found" >&2
  exit 1
fi
xdg-mime default omapdf.desktop application/pdf
echo "default application/pdf -> $(xdg-mime query default application/pdf)"

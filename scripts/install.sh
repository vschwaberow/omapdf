#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${BUILD_DIR:-$root/build}"
build_type="${BUILD_TYPE:-Release}"
prefix="${PREFIX:-$HOME/.local}"

cmake -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE="$build_type" "$root"
cmake --build "$build_dir"
cmake --install "$build_dir" --prefix "$prefix"

update-desktop-database "$prefix/share/applications"
gtk-update-icon-cache -f "$prefix/share/icons/hicolor" 2>/dev/null || true

echo "installed: $prefix/bin/omapdf"

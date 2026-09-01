#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${BUILD_DIR:-$root/build-debug}"
build_type="${BUILD_TYPE:-Debug}"
asan="${OMAPDF_ASAN:-OFF}"

export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"

cmake -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE="$build_type" -DOMAPDF_ASAN="$asan" "$root"
cmake --build "$build_dir"
ctest --test-dir "$build_dir" --output-on-failure "$@"

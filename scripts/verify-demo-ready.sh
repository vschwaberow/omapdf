#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
fail=0
need() {
  if [[ ! -e "$1" ]]; then
    echo "MISSING $1"
    fail=1
  else
    echo "OK $1"
  fi
}
need "$root/build/omapdf"
need "$root/scripts/set-default-pdf-handler.sh"
need "$root/packaging/omapdf.desktop"
need "$root/docs/demo-script.md"
need "$root/scripts/verify-l1-beauty.sh"
"$root/scripts/verify-l1-beauty.sh" || fail=1
ver="$("$root/build/omapdf" --version 2>/dev/null || true)"
echo "version: $ver"
if [[ "$ver" != *0.2.3* ]]; then
  echo "EXPECTED omapdf 0.2.3"
  fail=1
fi
exit "$fail"

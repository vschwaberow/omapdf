#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
theme="${HOME}/.local/state/omarchy/current/theme"
fail=0
need() { if [[ ! -e "$1" ]]; then echo "MISSING $1"; fail=1; else echo "OK $1"; fi; }
need "$theme/colors.toml"
need "$theme/shell.toml"
need "$root/qml/Main.qml"
need "$root/qml/chrome/ToolRail.qml"
need "$root/qml/viewer/WelcomePage.qml"
need "$root/assets/omapdf-logo.svg"
need "$root/src/app/ThemeBridge.cpp"
if ! rg -q 'sideChromeBudget: Math.max\(200, Math.floor\(width \* 0\.28\)\)' "$root/qml/Main.qml"; then
  echo "MISSING sidebar budget 0.28"
  fail=1
else
  echo "OK sidebar budget 0.28"
fi
if ! rg -q 'reload\(\)|colors\.toml' "$root/src/app/ThemeBridge.cpp"; then
  echo "MISSING ThemeBridge colors reload"
  fail=1
else
  echo "OK ThemeBridge colors reload"
fi
exit "$fail"

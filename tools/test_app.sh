#!/bin/bash
# test_app.sh — put a LOCAL test copy of the current build on J's Desktop.
# Why not just copy dist/JellyDazzle.app: it is Developer-ID signed but NOT
# notarized, and current macOS refuses to spawn that ("Launchd job spawn failed",
# POSIX 163) wherever it sits — the "dist build won't launch" mystery. An ad-hoc
# signature is treated as a locally built app and runs. Never ship this copy:
# releases go through release_app.sh (notarized + stapled) or build_appstore.sh.
set -euo pipefail
cd "$(dirname "$0")/.."
bash tools/build_app.sh >/dev/null
VER=$(cat VERSION); D="$HOME/Desktop/JellyDazzle ${VER%.0} (test).app"
rm -rf "$D"; ditto dist/JellyDazzle.app "$D"; xattr -cr "$D"
codesign --force --deep -s - "$D" >/dev/null 2>&1
codesign --verify --deep --strict "$D" && echo "test app ready: $D"

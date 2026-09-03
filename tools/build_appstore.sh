#!/usr/bin/env bash
# build_appstore.sh — package JellyDazzle for the MAC APP STORE.
#
# This is NOT the same pipeline as build_app.sh + release_app.sh:
#
#   build_app.sh    -> ad-hoc signed .app for local use
#   release_app.sh  -> "Developer ID Application" + notarisation, for direct
#                      download from your own site. No sandbox, no review.
#   THIS SCRIPT     -> "Apple Distribution" + App Sandbox + a signed .pkg,
#                      uploaded to App Store Connect. Sandbox is MANDATORY and
#                      a Developer ID signature is REJECTED here.
#
# Requires (all appear once LIBCSYSTEMS LLC is approved and you download the
# certificates in Xcode > Settings > Accounts > Manage Certificates):
#   * "Apple Distribution: ... (TEAMID)"              - signs the .app
#   * "3rd Party Mac Developer Installer: ... (TEAMID)" - signs the .pkg
set -euo pipefail
cd "$(dirname "$0")/.."

VER=$(cat VERSION)
APP=dist/JellyDazzle.app
PKG="dist/JellyDazzle-${VER}.pkg"
ENTITLEMENTS=packaging/JellyDazzle.entitlements

[ -f "$ENTITLEMENTS" ] || { echo "ERROR: missing $ENTITLEMENTS" >&2; exit 1; }

# --- locate the two certificates -------------------------------------------
# Fail loudly and specifically. "codesign: no identity found" three steps later
# is a much worse experience than being told which cert is missing up front.
APP_ID=$(security find-identity -v -p codesigning \
         | grep -E '"(Apple Distribution|3rd Party Mac Developer Application)' \
         | head -1 | sed -E 's/.*"([^"]+)".*/\1/' || true)
PKG_ID=$(security find-identity -v \
         | grep -E '"3rd Party Mac Developer Installer' \
         | head -1 | sed -E 's/.*"([^"]+)".*/\1/' || true)

if [ -z "$APP_ID" ]; then
  echo "ERROR: no 'Apple Distribution' certificate in the keychain." >&2
  echo "  Xcode > Settings > Accounts > Manage Certificates > + Apple Distribution" >&2
  echo "  (A 'Developer ID Application' cert will NOT work for the App Store.)" >&2
  exit 1
fi
if [ -z "$PKG_ID" ]; then
  echo "ERROR: no '3rd Party Mac Developer Installer' certificate in the keychain." >&2
  echo "  Create it at developer.apple.com > Certificates > Mac Installer Distribution" >&2
  exit 1
fi
echo "app signing : $APP_ID"
echo "pkg signing : $PKG_ID"

# --- build the bundle exactly as the normal path does ----------------------
# Reusing build_app.sh keeps the Info.plist, icon and SDL2 vendoring in ONE
# place. It ad-hoc signs; everything below re-signs properly over the top.
tools/build_app.sh >/dev/null

# --- strip quarantine ------------------------------------------------------
# Anything that has been downloaded or dragged carries com.apple.quarantine,
# and App Store Connect has rejected uploads over it since Feb 2025. build_app.sh
# already does this for the bundle; repeat it here so this script is safe to run
# standalone, and cover the packaging inputs too.
xattr -cr "$APP" packaging 2>/dev/null || true

# --- re-sign inside-out with the real identity + entitlements ---------------
# Order matters: nested code first, container last. --deep is deprecated and
# seals unreliably, which is why each item is signed explicitly.
# The sandbox entitlement goes on the MAIN EXECUTABLE, not the dylib.
if [ -f "$APP/Contents/Frameworks/libSDL3.dylib" ]; then
  codesign --force --timestamp --options runtime -s "$APP_ID" \
           "$APP/Contents/Frameworks/libSDL3.dylib"
fi
codesign --force --timestamp --options runtime -s "$APP_ID" \
         "$APP/Contents/Frameworks/libSDL2-2.0.0.dylib"
codesign --force --timestamp --options runtime -s "$APP_ID" \
         --entitlements "$ENTITLEMENTS" "$APP/Contents/MacOS/JellyDazzle"
codesign --force --timestamp --options runtime -s "$APP_ID" \
         --entitlements "$ENTITLEMENTS" "$APP"

# --- verify BEFORE packaging ----------------------------------------------
codesign --verify --strict --verbose=2 "$APP"
echo "--- entitlements actually embedded ---"
codesign -d --entitlements - "$APP" 2>/dev/null | grep -E 'app-sandbox|audio-input' \
  || { echo "ERROR: sandbox entitlement did not embed - the store will reject this." >&2; exit 1; }

# --- build the installer package ------------------------------------------
rm -f "$PKG"
productbuild --component "$APP" /Applications --sign "$PKG_ID" "$PKG"

echo
echo "built $PKG"
echo
echo "Next:"
echo "  1. Validate and upload with Transporter (App Store Connect > Apps > + )"
echo "     or:  xcrun altool --validate-app -f \"$PKG\" -t macos --apiKey ... --apiIssuer ..."
echo "  2. Uploads must come from Xcode 26 or later toolchains (enforced 2026-04-28)."
echo "  3. In App Store Connect, complete the age-rating questionnaire and EU"
echo "     trader status before the build can be submitted for review."

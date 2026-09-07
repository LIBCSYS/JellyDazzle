#!/bin/bash
# release_app.sh — build, Developer-ID sign, notarize, staple, zip.
# Produces a JellyDazzle.app that downloads and runs with NO warnings.
#
# One-time setup (see README "Signed releases"):
#   1. Xcode > Settings > Accounts > Manage Certificates > + Developer ID Application
#   2. xcrun notarytool store-credentials jellydazzle \
#        --apple-id <your-apple-id> --team-id <TEAMID> --password <app-specific-password>
# Then:  ./tools/release_app.sh
set -e
cd "$(dirname "$0")/.."
VER=$(cat VERSION)
APP=dist/JellyDazzle.app
PROFILE="${NOTARY_PROFILE:-jellydazzle}"

IDENTITY=$(security find-identity -v -p codesigning | grep "Developer ID Application" | head -1 |
           sed -E 's/.*"(Developer ID Application: .*)"/\1/')
if [ -z "$IDENTITY" ]; then
  echo "ERROR: no 'Developer ID Application' certificate in the keychain." >&2
  echo "  Xcode > Settings > Accounts > Manage Certificates > + Developer ID Application" >&2
  exit 1
fi
echo "signing identity: $IDENTITY"

./tools/build_app.sh >/dev/null        # fresh unsigned bundle + libs

# sign inside-out, hardened runtime + secure timestamp (notarization requires both)
# libSDL3 is not vendored. build_app.sh already guards this; this script did not,
# so it aborted here under set -e and never reached the notarisation step - which
# is why no release was ever produced despite the script "running".
if [ -f "$APP/Contents/Frameworks/libSDL3.dylib" ]; then
    codesign --force --options runtime --timestamp \
        --sign "$IDENTITY" "$APP/Contents/Frameworks/libSDL3.dylib"
fi
codesign --force --options runtime --timestamp \
    --sign "$IDENTITY" "$APP/Contents/Frameworks/libSDL2-2.0.0.dylib"
# Entitlements MUST be repeated on every re-sign. Omit them here and the
# hardened runtime strips microphone access, so the app builds, ships, and
# then silently fails to react to audio - the one thing it does.
codesign --force --options runtime --timestamp \
    --entitlements packaging/JellyDazzle.entitlements \
    --sign "$IDENTITY" "$APP/Contents/MacOS/JellyDazzle"
codesign --force --options runtime --timestamp \
    --entitlements packaging/JellyDazzle.entitlements \
    --sign "$IDENTITY" "$APP"
codesign --verify --deep --strict --verbose=2 "$APP"

# GUARD. On 2026-09-05 a JellyDazzle-3.0.2.pkg was submitted here and came back
# Invalid: "The binary is not signed with a valid Developer ID certificate."
# That pkg was the APP STORE artifact, signed with Apple Distribution.
#
# The three destinations take three different certificates and only ONE of them
# is notarised by you:
#   direct download  -> Developer ID Application -> notarise here    (this script)
#   Mac App Store    -> Apple Distribution       -> upload, Apple notarises
#   local testing    -> ad-hoc                   -> never leaves the machine
#
# Refuse to submit anything that is not Developer ID rather than burning five
# minutes to be told so by Apple.
AUTH=$(codesign -dvv "$APP" 2>&1 | grep -m1 "^Authority=" | sed 's/^Authority=//')
case "$AUTH" in
  "Developer ID Application"*) : ;;
  *) echo "ERROR: refusing to notarise - the bundle is signed as:" >&2
     echo "         ${AUTH:-(unsigned or ad-hoc)}" >&2
     echo "       Notarisation requires 'Developer ID Application'." >&2
     echo "       If you meant the App Store, use tools/build_appstore.sh and" >&2
     echo "       upload the .pkg - Apple notarises that side, not you." >&2
     exit 1 ;;
esac
echo "verified signing authority: $AUTH"

rm -f dist/JellyDazzle-notarize.zip
ditto -c -k --keepParent "$APP" dist/JellyDazzle-notarize.zip
echo "submitting to Apple for notarization (usually 1-5 minutes)..."
xcrun notarytool submit dist/JellyDazzle-notarize.zip \
    --keychain-profile "$PROFILE" --wait
xcrun stapler staple "$APP"           # embed the ticket: works offline
xcrun stapler validate "$APP"
spctl -a -vv "$APP"                    # must say: accepted / Notarized Developer ID

rm -f dist/JellyDazzle.app.zip dist/JellyDazzle-notarize.zip
(cd dist && ditto -c -k --keepParent JellyDazzle.app JellyDazzle.app.zip)
echo "JellyDazzle v${VER} signed + notarized + stapled -> dist/JellyDazzle.app.zip"
echo "ship it:  gh release create v${VER} dist/JellyDazzle.app.zip --repo LIBCSYS/JellyDazzle"

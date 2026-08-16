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
codesign --force --options runtime --timestamp \
    --sign "$IDENTITY" "$APP/Contents/Frameworks/libSDL3.dylib"
codesign --force --options runtime --timestamp \
    --sign "$IDENTITY" "$APP/Contents/Frameworks/libSDL2-2.0.0.dylib"
codesign --force --options runtime --timestamp \
    --sign "$IDENTITY" "$APP/Contents/MacOS/JellyDazzle"
codesign --force --options runtime --timestamp \
    --sign "$IDENTITY" "$APP"
codesign --verify --deep --strict --verbose=2 "$APP"

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

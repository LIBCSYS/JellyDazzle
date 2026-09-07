#!/bin/bash
# build_app.sh — build dist/JellyDazzle.app, versioned from ./VERSION
set -e
cd "$(dirname "$0")/.."
VER=$(cat VERSION)
# Read MACMIN from the Makefile rather than repeating it. These two disagreeing
# is exactly how a build ships claiming macOS 11 while refusing to launch on it.
# Environment wins. The App Store build passes MACMIN=12.0 because Apple
# rejects an arm64-only bundle below that (error 90869), and it reads
# LSMinimumSystemVersion from the plist written here — so this value and the
# compiler's -mmacosx-version-min must agree or the upload fails while the
# binary looks correct.
if [ -z "${MACMIN:-}" ]; then
    MACMIN=$(awk -F= '/^MACMIN/{gsub(/[ ?]/,"",$2); print $2}' Makefile)
    MACMIN=${MACMIN:-11.0}
fi
APP=dist/JellyDazzle.app
make
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Frameworks" "$APP/Contents/Resources"
if [ -f assets/brand/JellyDazzle.icns ]; then
    cp assets/brand/JellyDazzle.icns "$APP/Contents/Resources/JellyDazzle.icns"
fi
cp jellydazzle "$APP/Contents/MacOS/JellyDazzle"
# sdl2-compat shim + the SDL3 it dlopens as @loader_path/libSDL3.dylib
SHIM=$(otool -L jellydazzle | awk '/libSDL2/{print $1}')
cp "$SHIM" "$APP/Contents/Frameworks/libSDL2-2.0.0.dylib"
# Homebrew ships sdl2-compat, a shim that dlopens SDL3 — bundling SDL3 was only
# ever to satisfy that. With a real SDL2 (vendor/sdl2, built against MACMIN)
# there is nothing to load, and the Homebrew SDL3 would drag the whole bundle
# back up to whatever macOS this machine runs.
if otool -L "$APP/Contents/Frameworks/libSDL2-2.0.0.dylib" | grep -q SDL3; then
    cp /opt/homebrew/opt/sdl3/lib/libSDL3.0.dylib "$APP/Contents/Frameworks/libSDL3.dylib"
fi
install_name_tool -change "$SHIM" @executable_path/../Frameworks/libSDL2-2.0.0.dylib \
    "$APP/Contents/MacOS/JellyDazzle"
if [ -f "$APP/Contents/Frameworks/libSDL3.dylib" ]; then
    install_name_tool -id @loader_path/libSDL3.dylib "$APP/Contents/Frameworks/libSDL3.dylib"
fi
chmod u+w "$APP/Contents/Frameworks/"*.dylib      # Homebrew ships them read-only
cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>JellyDazzle</string>
  <key>CFBundleDisplayName</key><string>JellyDazzle</string>
  <key>CFBundleExecutable</key><string>JellyDazzle</string>
  <key>CFBundleIdentifier</key><string>nyc.jelia.jellydazzle</string>
  <key>CFBundleVersion</key><string>${VER}</string>
  <key>CFBundleShortVersionString</key><string>${VER}</string>
  <key>CFBundleIconFile</key><string>JellyDazzle</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <!-- Required by the App Store; also what Finder/Launchpad categorise by. -->
  <key>LSApplicationCategoryType</key><string>public.app-category.entertainment</string>
  <!-- Must match the compiler's -mmacosx-version-min. Without it macOS infers
       the SDK version and the app refuses to launch on older systems. -->
  <key>LSMinimumSystemVersion</key><string>${MACMIN}</string>
  <key>NSHumanReadableCopyright</key><string>Copyright © 2026 John Elia / LIBCSYSTEMS LLC. MIT licensed.</string>
  <key>NSHighResolutionCapable</key><true/>
  <key>NSAudioCaptureUsageDescription</key><string>JellyDazzle listens to what the Mac is playing (Spotify, any app) so the kaleidoscope can move with the music. Audio is analysed in memory only — never recorded, stored, or sent anywhere.</string>
  <key>NSMicrophoneUsageDescription</key><string>JellyDazzle listens to whatever it can hear so the kaleidoscope can move with the music. Audio is analysed in memory only — never recorded, stored, or sent anywhere.</string>
</dict></plist>
PLIST
# strip stray xattrs BEFORE signing: they become ._AppleDouble files inside
# the bundle on unzip and break the seal ("a sealed resource is missing")
xattr -cr "$APP"
# sign inside-out (--deep is deprecated and seals unreliably)
# SDL3 is only present when the SDL2 we linked is Homebrew's sdl2-compat shim.
# With the vendored real SDL2 there is nothing to sign here, and signing a file
# that does not exist aborted the whole script under `set -e`.
# SIGNING. This used to hard-code `-s -` (ad-hoc), which was correct while the
# Apple enrolment was still pending and wrong the moment the certificates
# arrived: an ad-hoc bundle is REJECTED by Gatekeeper, so double-clicking the
# download does nothing but show a scary dialog.
#
# Prefer the real Developer ID identity when the keychain has one; fall back to
# ad-hoc so the script still works on a machine without the certificate.
ID=$(security find-identity -v -p codesigning 2>/dev/null      | grep "Developer ID Application" | head -1      | sed -E 's/.*"(.*)"/\1/')
if [ -n "$ID" ]; then
    echo "signing as: $ID"
    # --options runtime is the hardened runtime, which notarisation REQUIRES.
    # Under it, microphone access needs the entitlement declared - the Info.plist
    # usage string alone is not enough.
    ENT="--entitlements packaging/JellyDazzle.entitlements"
else
    echo "WARNING: no Developer ID Application certificate found - signing ad-hoc."
    echo "         Gatekeeper will reject the result. Fine for local testing only."
    ENT=""
fi

# A FUNCTION, not a variable. The identity string contains spaces, so holding
# the whole command in a variable word-splits it and codesign ends up looking
# for a file called "ID:".
sign_it () {
    if [ -n "$ID" ]; then
        codesign --force --timestamp --options runtime -s "$ID" "$@"
    else
        codesign --force -s - "$@"
    fi
}

if [ -f "$APP/Contents/Frameworks/libSDL3.dylib" ]; then
    sign_it "$APP/Contents/Frameworks/libSDL3.dylib"
fi
# Libraries are signed WITHOUT entitlements; only the main executable and the
# bundle carry them. Signing a dylib with entitlements is invalid.
sign_it "$APP/Contents/Frameworks/libSDL2-2.0.0.dylib"
sign_it $ENT "$APP/Contents/MacOS/JellyDazzle"
sign_it $ENT "$APP"
codesign --verify --deep --strict "$APP"
rm -f dist/JellyDazzle.app.zip
# --sequesterRsrc keeps metadata out of the bundle tree on extraction
(cd dist && ditto -c -k --sequesterRsrc --keepParent JellyDazzle.app JellyDazzle.app.zip)
echo "built JellyDazzle v${VER} -> $APP (+ .zip)"

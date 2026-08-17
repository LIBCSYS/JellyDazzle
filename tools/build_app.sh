#!/bin/bash
# build_app.sh — build dist/JellyDazzle.app, versioned from ./VERSION
set -e
cd "$(dirname "$0")/.."
VER=$(cat VERSION)
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
if [ -f "$APP/Contents/Frameworks/libSDL3.dylib" ]; then
    codesign --force -s - "$APP/Contents/Frameworks/libSDL3.dylib"
fi
codesign --force -s - "$APP/Contents/Frameworks/libSDL2-2.0.0.dylib"
codesign --force -s - "$APP/Contents/MacOS/JellyDazzle"
codesign --force -s - "$APP"
codesign --verify --deep --strict "$APP"
rm -f dist/JellyDazzle.app.zip
# --sequesterRsrc keeps metadata out of the bundle tree on extraction
(cd dist && ditto -c -k --sequesterRsrc --keepParent JellyDazzle.app JellyDazzle.app.zip)
echo "built JellyDazzle v${VER} -> $APP (+ .zip)"

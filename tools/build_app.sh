#!/bin/bash
# build_app.sh — build dist/JellyDazzle.app, versioned from ./VERSION
set -e
cd "$(dirname "$0")/.."
VER=$(cat VERSION)
APP=dist/JellyDazzle.app
make
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Frameworks"
cp jellydazzle "$APP/Contents/MacOS/JellyDazzle"
# sdl2-compat shim + the SDL3 it dlopens as @loader_path/libSDL3.dylib
SHIM=$(otool -L jellydazzle | awk '/libSDL2/{print $1}')
cp "$SHIM" "$APP/Contents/Frameworks/libSDL2-2.0.0.dylib"
cp /opt/homebrew/opt/sdl3/lib/libSDL3.0.dylib "$APP/Contents/Frameworks/libSDL3.dylib"
install_name_tool -change "$SHIM" @executable_path/../Frameworks/libSDL2-2.0.0.dylib \
    "$APP/Contents/MacOS/JellyDazzle"
install_name_tool -id @loader_path/libSDL3.dylib "$APP/Contents/Frameworks/libSDL3.dylib"
chmod u+w "$APP/Contents/Frameworks/"*.dylib      # Homebrew ships them read-only
cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>JellyDazzle</string>
  <key>CFBundleDisplayName</key><string>JellyDazzle</string>
  <key>CFBundleExecutable</key><string>JellyDazzle</string>
  <key>CFBundleIdentifier</key><string>nyc.jelia.jd${VER}</string>
  <key>CFBundleVersion</key><string>${VER}</string>
  <key>CFBundleShortVersionString</key><string>${VER}</string>
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
codesign --force -s - "$APP/Contents/Frameworks/libSDL3.dylib"
codesign --force -s - "$APP/Contents/Frameworks/libSDL2-2.0.0.dylib"
codesign --force -s - "$APP/Contents/MacOS/JellyDazzle"
codesign --force -s - "$APP"
codesign --verify --deep --strict "$APP"
rm -f dist/JellyDazzle.app.zip
# --sequesterRsrc keeps metadata out of the bundle tree on extraction
(cd dist && ditto -c -k --sequesterRsrc --keepParent JellyDazzle.app JellyDazzle.app.zip)
echo "built JellyDazzle v${VER} -> $APP (+ .zip)"

#!/bin/bash
# build_app.sh — build dist/JellyDazzle.app, versioned from ./VERSION
set -e
cd "$(dirname "$0")/.."
VER=$(cat VERSION)
APP=dist/JellyDazzle.app
make
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Frameworks"
cp dazzle64 "$APP/Contents/MacOS/JellyDazzle"
# sdl2-compat shim + the SDL3 it dlopens as @loader_path/libSDL3.dylib
SHIM=$(otool -L dazzle64 | awk '/libSDL2/{print $1}')
cp "$SHIM" "$APP/Contents/Frameworks/libSDL2-2.0.0.dylib"
cp /opt/homebrew/opt/sdl3/lib/libSDL3.0.dylib "$APP/Contents/Frameworks/libSDL3.dylib"
install_name_tool -change "$SHIM" @executable_path/../Frameworks/libSDL2-2.0.0.dylib \
    "$APP/Contents/MacOS/JellyDazzle"
install_name_tool -id @loader_path/libSDL3.dylib "$APP/Contents/Frameworks/libSDL3.dylib"
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
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
codesign --force --deep -s - "$APP"
rm -f dist/JellyDazzle.app.zip
(cd dist && ditto -c -k --keepParent JellyDazzle.app JellyDazzle.app.zip)
echo "built JellyDazzle v${VER} -> $APP (+ .zip)"

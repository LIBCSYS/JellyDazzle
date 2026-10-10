#!/bin/bash
# tools/build_ios.sh — JellyDazzle for iPhone and iPad (3.5.1), no Xcode project.
#
#   tools/build_ios.sh sim                 simulator .app, ad-hoc signed   -> dist/ios-sim/
#   BUILD=6 tools/build_ios.sh store       App Store .ipa (Apple Distribution + iOS profile)
#                                                                          -> dist/ios-store/
#
# Same idea as build_appstore.sh on the Mac: one clang pass over the same
# sources, so the engine, the assembly and all 610 patterns are byte-for-byte
# the Mac's. What differs is only the shim (main.c's TARGET_OS_IPHONE paths +
# src/app/ios_host.m) and SDL, which is vendor/sdl2-ios (2.30.9, built from
# source with one audio-session patch — see vendor/sdl2-ios/*.patch).
#
# ⚠ BUILD numbers are burned forever by App Store Connect, per app record, and
#   the Mac and iOS builds share this record. Use plain integers above the
#   highest ever uploaded (3.0.5 as of 2026-10-08): 6, 7, 8 ...
set -euo pipefail
cd "$(dirname "$0")/.."

MODE=${1:-sim}
VERSION=$(cat VERSION)
BUILD=${BUILD:-}
IOSMIN=15.0
BUNDLE_ID=nyc.jelia.jellydazzle
TEAM=46AXDWA6F8

case "$MODE" in
  sim)   SDK=iphonesimulator; TRIPLE=arm64-apple-ios$IOSMIN-simulator; PLAT=iPhoneSimulator; BUILD=${BUILD:-$VERSION} ;;
  store) SDK=iphoneos;        TRIPLE=arm64-apple-ios$IOSMIN;           PLAT=iPhoneOS
         [[ "$BUILD" =~ ^[0-9]+$ ]] || { echo "store build needs BUILD=<integer>, higher than any uploaded" >&2; exit 1; } ;;
  *) echo "usage: $0 sim|store" >&2; exit 1 ;;
esac

SYSROOT=$(xcrun --sdk $SDK --show-sdk-path)
OUT=dist/ios-$MODE
APP=$OUT/JellyDazzle.app
rm -rf "$OUT"; mkdir -p "$APP" "$OUT/tmp"

# ---- 1. compile + link (one pass, like the Makefile) ----------------------
NS=$(awk '/define JD_SCHEMES/{print $3}' src/engine/palette_count.h)
PATTERNS=$(ls src/patterns/[0-9]*.c | grep -v _harness.c)
echo "compiling for $SDK ($TRIPLE)…"
xcrun --sdk $SDK clang -O2 -target "$TRIPLE" -isysroot "$SYSROOT" \
  -Isrc/engine -Ivendor/sdl2-ios/include \
  -DJD_VERSION="\"$VERSION\"" -DJD_NS="$NS" \
  src/app/main.c src/audio/listen.c src/engine/compositor.c src/engine/routines_asm.s \
  $PATTERNS src/patterns/_registry.c \
  -fobjc-arc src/app/ios_host.m \
  vendor/sdl2-ios/$SDK/libSDL2.a \
  -framework UIKit -framework Foundation -framework CoreGraphics -framework QuartzCore \
  -framework Metal -framework OpenGLES -framework AVFoundation -framework AudioToolbox \
  -framework CoreAudio -framework CoreMotion -framework GameController -framework CoreHaptics \
  -framework CoreBluetooth \
  -o "$APP/JellyDazzle" 2>&1 | grep -E "error|Undefined|ld: " || true
[ -x "$APP/JellyDazzle" ] || { echo "link failed" >&2; exit 1; }

# ---- 2. icon: single-size 1024 asset catalog -> Assets.car ------------------
XC=$OUT/tmp/Assets.xcassets/AppIcon.appiconset
mkdir -p "$XC"
cp assets/brand/icon-1024.png "$XC/icon-1024.png"     # full-bleed, no alpha: App Store rule
cat > "$OUT/tmp/Assets.xcassets/Contents.json" <<'J'
{ "info" : { "author" : "xcode", "version" : 1 } }
J
cat > "$XC/Contents.json" <<'J'
{ "images" : [ { "filename" : "icon-1024.png", "idiom" : "universal", "platform" : "ios", "size" : "1024x1024" } ],
  "info" : { "author" : "xcode", "version" : 1 } }
J
xcrun actool "$OUT/tmp/Assets.xcassets" --compile "$APP" --platform $SDK \
  --minimum-deployment-target $IOSMIN --target-device iphone --target-device ipad \
  --app-icon AppIcon --output-partial-info-plist "$OUT/tmp/icon.plist" >/dev/null

# ---- 3. Info.plist ---------------------------------------------------------
# DT* keys are what Xcode stamps; App Store validation reads them, so they are
# filled from the real toolchain rather than hard-coded.
SDKVER=$(xcrun --sdk $SDK --show-sdk-version)
SDKBUILD=$(xcrun --sdk $SDK --show-sdk-build-version)
XCV=$(xcodebuild -version | awk 'NR==1{print $2}')
XCB=$(xcodebuild -version | awk 'NR==2{print $3}')
XCODE_NUM=$(echo "$XCV" | awk -F. '{printf "%02d%d%d", $1, $2, $3}')
cat > "$APP/Info.plist" <<P
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key><string>en</string>
  <key>CFBundleExecutable</key><string>JellyDazzle</string>
  <key>CFBundleIdentifier</key><string>$BUNDLE_ID</string>
  <key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
  <key>CFBundleName</key><string>JellyDazzle</string>
  <key>CFBundleDisplayName</key><string>JellyDazzle</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>$VERSION</string>
  <key>CFBundleVersion</key><string>$BUILD</string>
  <key>CFBundleSupportedPlatforms</key><array><string>$PLAT</string></array>
  <key>CFBundleIconName</key><string>AppIcon</string>
  <key>LSRequiresIPhoneOS</key><true/>
  <key>MinimumOSVersion</key><string>$IOSMIN</string>
  <key>UIDeviceFamily</key><array><integer>1</integer><integer>2</integer></array>
  <key>UIRequiredDeviceCapabilities</key><array><string>arm64</string></array>
  <!-- An empty UILaunchScreen is what makes iOS run the app at the device's real
       resolution; without any launch screen it is letterboxed to a legacy size. -->
  <key>UILaunchScreen</key><dict/>
  <key>UIStatusBarHidden</key><true/>
  <key>UIRequiresFullScreen</key><true/>
  <key>UIApplicationSupportsIndirectInputEvents</key><true/>
  <!-- iOS 27 kills apps without the UIScene lifecycle at launch (3.5.1 build 6 on
       J's iPhone 15 Pro Max: SIGTRAP in _UIApplicationEvaluateRuntimeIssueFor
       NoSceneLifecycleAdoption; the simulator only warned). One scene, delegate
       class added to SDL by vendor/sdl2-ios/sdl-jellydazzle-uiscene.patch. -->
  <key>UIApplicationSceneManifest</key>
  <dict>
    <key>UIApplicationSupportsMultipleScenes</key><false/>
    <key>UISceneConfigurations</key>
    <dict>
      <key>UIWindowSceneSessionRoleApplication</key>
      <array><dict>
        <key>UISceneConfigurationName</key><string>Default</string>
        <key>UISceneDelegateClassName</key><string>SDLUIKitSceneDelegate</string>
      </dict></array>
    </dict>
  </dict>
  <key>UISupportedInterfaceOrientations</key><array>
    <string>UIInterfaceOrientationPortrait</string><string>UIInterfaceOrientationLandscapeLeft</string>
    <string>UIInterfaceOrientationLandscapeRight</string></array>
  <key>UISupportedInterfaceOrientations~ipad</key><array>
    <string>UIInterfaceOrientationPortrait</string><string>UIInterfaceOrientationPortraitUpsideDown</string>
    <string>UIInterfaceOrientationLandscapeLeft</string><string>UIInterfaceOrientationLandscapeRight</string></array>
  <key>NSMicrophoneUsageDescription</key>
  <string>JellyDazzle listens to the music playing around you so the patterns move with it. Nothing is recorded, saved or sent anywhere.</string>
  <!-- SDL links CoreBluetooth for game controllers; iOS requires the string even though JellyDazzle never scans -->
  <key>NSBluetoothAlwaysUsageDescription</key>
  <string>JellyDazzle does not use Bluetooth. This permission is never requested.</string>
  <key>ITSAppUsesNonExemptEncryption</key><false/>
  <key>DTPlatformName</key><string>$SDK</string>
  <key>DTPlatformVersion</key><string>$SDKVER</string>
  <key>DTPlatformBuild</key><string>$SDKBUILD</string>
  <key>DTSDKName</key><string>$SDK$SDKVER</string>
  <key>DTSDKBuild</key><string>$SDKBUILD</string>
  <key>DTXcode</key><string>$XCODE_NUM</string>
  <key>DTXcodeBuild</key><string>$XCB</string>
  <key>DTCompiler</key><string>com.apple.compilers.llvm.clang.1_0</string>
  <key>BuildMachineOSBuild</key><string>$(sw_vers -buildVersion)</string>
</dict>
</plist>
P
# merge what actool says the icon needs (CFBundleIcons etc.)
/usr/libexec/PlistBuddy -c "Merge $OUT/tmp/icon.plist" "$APP/Info.plist" >/dev/null
plutil -lint "$APP/Info.plist" >/dev/null

# ---- 4. privacy manifest (required by App Store since 2024) ----------------
cp packaging/ios/PrivacyInfo.xcprivacy "$APP/"

# ---- 5. sign ---------------------------------------------------------------
if [ "$MODE" = sim ]; then
  codesign --force --sign - --timestamp=none "$APP"
else
  PROFILE=packaging/ios/JellyDazzle_iOS_AppStore.mobileprovision
  [ -f "$PROFILE" ] || { echo "missing $PROFILE (iOS App Store profile)" >&2; exit 1; }
  cp "$PROFILE" "$APP/embedded.mobileprovision"
  # Same check as the Mac pipeline (ITMS-90303): signing with a drifted clock
  # gets the upload rejected. Refuse to sign more than 5 s off Apple time.
  # sntp sometimes returns an error line instead of an offset (seen twice on
  # 2026-10-08, seconds after a clean reading) — retry before refusing.
  OFF=""
  for try in 1 2 3; do
    OFF=$(sntp time.apple.com 2>/dev/null | awk '$1 ~ /^[+-]?[0-9.]+$/ {print $1}' | tr -d '+' | head -1)
    [ -n "$OFF" ] && break; sleep 2
  done
  [ -n "$OFF" ] || { echo "could not read Apple time after 3 tries — not signing" >&2; exit 1; }
  awk -v o="${OFF:-0}" 'BEGIN{exit (o<5 && o>-5)?0:1}' || { echo "clock is ${OFF}s off Apple time — fix before signing" >&2; exit 1; }
  # By fingerprint: three "Apple Distribution: LIBCSYSTEMS LLC" identities live in
  # the keychain and a name match is ambiguous. This one (serial 7D38F18A…, ASC cert
  # 246D7W6UWJ) is the one both the Mac and iOS profiles were issued against.
  codesign --force --sign 10CA3AB124AB2F2430FF39C3468813F4E27B4783 \
    --entitlements packaging/ios/JellyDazzle-iOS.entitlements --timestamp "$APP"
  mkdir -p "$OUT/ipa/Payload"; cp -R "$APP" "$OUT/ipa/Payload/"
  (cd "$OUT/ipa" && zip -qry "../JellyDazzle-$VERSION-$BUILD.ipa" Payload)
  echo "ipa: $OUT/JellyDazzle-$VERSION-$BUILD.ipa"
fi
codesign --verify --strict "$APP" && echo "ok: $APP  ($VERSION build $BUILD, $(du -sh "$APP" | cut -f1))"

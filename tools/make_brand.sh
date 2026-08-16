#!/bin/bash
# make_brand.sh — turn one logo image into every asset the project needs.
#
#   tools/make_brand.sh path/to/logo.png
#
# Produces, in assets/brand/:
#   jellydazzle-logo.<ext>   the master, copied verbatim
#   emblem.png               square centre crop (the JD mark, no wordmark)
#   icon-1024.png            1024 square, for stores and readmes
#   favicon-64.png           browser tab
#   social-1280x640.png      GitHub social preview / OpenGraph card
#   JellyDazzle.icns         macOS app icon, wired into the .app by build_app.sh
#
# NOTE: `sips -z` preserves the SOURCE encoding regardless of the output
# extension, so resizing a JPEG to "foo.png" yields JPEG bytes named .png and
# iconutil rejects the iconset with a bare "Failed to generate ICNS".  Every
# resize below is therefore followed by an explicit `-s format png`.
#
# The emblem crop is explicit, not centred: the wordmark sits above the mark,
# so a plain centre crop slices through the letters and looks like garbage at
# 32 px.  Override per logo:
#   tools/make_brand.sh logo.png [SIZE] [TOP] [LEFT]
set -e
cd "$(dirname "$0")/.."
SRC="${1:?usage: tools/make_brand.sh <logo image> [size] [top] [left]}"
[ -f "$SRC" ] || { echo "no such file: $SRC" >&2; exit 1; }
CROP=${2:-330}      # square side, source pixels
CTOP=${3:-172}      # offset from the top
CLEFT=${4:-347}     # offset from the left
B=assets/brand
mkdir -p "$B"

EXT="${SRC##*.}"
cp "$SRC" "$B/jellydazzle-logo.$EXT"
echo "master     -> $B/jellydazzle-logo.$EXT"

png() { sips -s format png "$1" --out "$1" >/dev/null 2>&1; }

# square crop centred on the emblem, then the derivatives
sips -c "$CROP" "$CROP" --cropOffset "$CTOP" "$CLEFT" "$SRC" \
     --out "$B/emblem.png" >/dev/null 2>&1; png "$B/emblem.png"
sips -z 1024 1024 "$B/emblem.png" --out "$B/icon-1024.png" >/dev/null 2>&1; png "$B/icon-1024.png"
sips -z 64 64 "$B/emblem.png" --out "$B/favicon-64.png" >/dev/null 2>&1; png "$B/favicon-64.png"
sips -z 640 1280 --padToHeightWidth 640 1280 --padColor 07080D "$SRC" \
     --out "$B/social-1280x640.png" >/dev/null 2>&1; png "$B/social-1280x640.png"
echo "derived    -> emblem, icon-1024, favicon-64, social-1280x640"

# macOS iconset -> .icns
IS="$B/JellyDazzle.iconset"
rm -rf "$IS"; mkdir -p "$IS"
for s in 16 32 128 256 512; do
    d=$((s * 2))
    sips -z $s $s "$B/icon-1024.png" --out "$IS/icon_${s}x${s}.png"     >/dev/null 2>&1
    sips -z $d $d "$B/icon-1024.png" --out "$IS/icon_${s}x${s}@2x.png"  >/dev/null 2>&1
    png "$IS/icon_${s}x${s}.png"; png "$IS/icon_${s}x${s}@2x.png"
done
iconutil -c icns "$IS" -o "$B/JellyDazzle.icns"
echo "icon       -> $B/JellyDazzle.icns ($(du -h "$B/JellyDazzle.icns" | cut -f1))"
echo "done."

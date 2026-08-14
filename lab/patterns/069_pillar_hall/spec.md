# 069 Pillar Hall

## Look
A first-person glide down an endless amber-and-mahogany hall: checkered floor and ceiling scroll toward you, side walls carry the same checker in a warmer key, and dark pillars sweep past at regular intervals while the camera sways and bobs like a slow walk. Warm candle-lit palette fading to black at the vanishing point.

## Math
- camera: `cx = W/2 + 16 sin(t*.004)`, `cy = H/2 + 7 sin(t*.0027)`
- plane pick: walls where `|dx| > |dy|*aspect`, else floor/ceiling
- `depth = 2400 / (|dx| or |dy|*aspect)` clamped; lateral `u = (other coord)/[chosen coord] * 34` (true perspective texture)
- march: `v = depth*.55 + t*.8`; checker `= (floor(u/16)+floor(v/20)) mod 2`
- pillars: wall pixels with `v mod 90 < 16` → luminance * 0.22
- `idx = depth*.55 + tile*46 + wall*36`; fog `= 1 - depth/460` → far end goes black

## Integer ARM64 plan
- The u = a/b divide is the one true per-pixel divide — kill it with the classic floorcaster trick: for each SCREEN ROW below/above the horizon, depth and the du step are constant → per-row: 1 recip_tab lookup + fixed du; inner loop is `u += du` (one add). Same per COLUMN for the walls. Total per-pixel: 1 add, 1 texture-byte fetch.
- Texture = 64x64 checker byte tile, wrap by AND. Pillar test = `(v & 127) < 22`-style mask compare on the per-row v byte, hoisted out of the inner loop.
- Sway/bob: 2 sin16 per frame shifting the horizon/center — the row tables just index differently, nothing recomputed.
- Fog & tile luminance folded into 4 prebuilt palettes (floor/wall x light/dark) selected per span; pillar spans use a 5th dark palette. Pure span-fill rendering, exactly how 1994 did corridors.

## Palette pairing
Amber loop: near-black → mahogany → amber → gold → cream → back. Walls sit +36 palette steps from the floor so the corner line reads architecturally; fog multiplies everything toward black so the vanishing point is a candle-lit darkness, not gray mush.

## Motion
Walk speed 0.8 v/frame (a pillar passes every ~11 s); sway period ≈ 52 s, bob ≈ 78 s — a calm stroll. No rotation, no strobe; the depth cue comes entirely from scroll-rate divergence between near and far texels.

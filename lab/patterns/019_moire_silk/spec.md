# 019 Moiré Silk

## Look
Two interference-ring sources orbiting inside an 8-fold wedge — sixteen mirrored wave sources on screen — whose beat patterns weave silky teal-and-ember moiré fabric with lens-like eyes along every seam. The looping palette rotates continuously, so waves appear to radiate outward forever.

## Math
- Fold: 8-fold wedge coords `(px,py)`; both source centers are themselves folded into the wedge.
- Centers: `c1 = (72·sin(0.006t), 56·sin(0.0043t+0.8))`, `c2 = (66·sin(0.0051t+2.1), 62·sin(0.0069t+4))`.
- Interference: `s = sin(0.30·d1 − 0.020t) + sin(0.26·d2 + 0.016t)` (soft addition, not XOR — silk, not fringes).
- Index: `idx = ((s+2)/4·255 + 0.40t) mod 256` into a looping 256-color LUT — palette rotation is the primary animation.

## Integer ARM64 plan
- Pure §4-moiré machinery: one oversized precomputed **distance byte table** (per-pixel distance from the center of a 704×624 virtual buffer); the two moving sources are just two window offsets `o1, o2` into it (computed per frame from `sin_tab`).
- Per pixel: `s = wave_tab[(dist[o1+i] + p1) & 255] + wave_tab[(dist[o2+i] + p2) & 255]` where `wave_tab` bakes the ring-frequency sine — 2 loads, 2 adds, 2 masks, 1 more add.
- Palette rotation: `pix = pal[(s + t·k) & 255]` — the +t is one register add; the LUT itself never changes. This is the closest pattern in the set to literal VGA DAC animation: on ARM we just rotate the palette pointer.
- The 8-fold happens in the `fold_map` used to build the per-pixel distance indices at init; frame loop is fold-free.

## Palette pairing
Looping five-stop silk ramp: deep teal → bright cyan → cream → ember orange → wine → teal. Complementary warm/cool pairing with a cream ridge — high richness, zero clash, and it tiles seamlessly under rotation.

## Motion
Palette rotates at 0.4 steps/frame (a full cycle ≈ 21 s — reads as waves lapping outward, well below strobe territory). Sources orbit on 15–24 s periods, so the interference figure slowly re-drapes; the seam "eyes" open and close as sources approach their reflections.

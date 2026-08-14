# 061 Checker Tunnel

## Look
A classic demoscene tunnel: rainbow spiral bands rush past the viewer down a dark throat at screen center, with a soft checkerboard dimming that gives the walls tile texture. The whole tube slowly rotates and the bands corkscrew as they fly by.

## Math
- `r = hypot(x-cx, y-cy)`, `a = atan2(y-cy, x-cx)`
- depth `v0 = K / (r + 14)` (K=5200) — the 1/r map that creates the tunnel illusion
- texture coords: `u = ang*3 + t*0.12 + 6*sin(v0*0.05 + t*0.01)` (angle 0..256, swirl wobble), `v = v0 + t*0.9` (fly)
- color index = `(u + v) mod 256` → looping rainbow palette (diagonal = spiral bands)
- checker luma: `((floor(u/21)+floor(v/21)) mod 2)*0.35 + 0.65`
- shade: `r/(r+60)` darkens the far end to black

## Integer ARM64 plan
- Precompute per-pixel byte LUTs at init: `ang_tab[i]` (angle 0..255) and `depth_tab[i]` (K/r clamped to byte). No per-pixel atan/div/sqrt at runtime.
- Frame loop: `u = ang3_tab[i] + rot; v = depth_tab[i] + fly;` (all uint8 adds, wrap free). `idx = (u+v) & 255` → palette LUT.
- Swirl wobble term folded into a second precomputed table sampled at `(depth>>3 + t>>4)` via sin16 table, added as a small signed byte.
- Checker = `((u>>~4) ^ (v>>~4)) & 1` selecting one of two prebuilt 256-entry palettes (bright/dim) — no multiply.
- Shade baked into a third per-pixel byte `shade_tab[i]`; final color via a 256×32 palette×brightness LUT (idx, shade>>3).

## Palette pairing
Looping saturated rainbow (violet→blue→teal→green→gold→red→violet). Shade table forces the center to black so the rainbow reads as lit tunnel walls. Palette rotation (add to idx) is a free extra motion channel.

## Motion
Bands fly toward the viewer at ~0.9 idx/frame (one full band cycle ≈ 9 s at 30 fps); tube rotates at 0.12 idx/frame; swirl wobble period ≈ 20 s. Nothing strobes — all three channels are slow phase adds.

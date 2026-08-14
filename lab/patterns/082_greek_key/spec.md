# 082 Greek Key

## Look
A central rectangular panel packed with interlocking greek-key / meander hooks (alternating
chirality per cell, exactly the a02–a04 maze-panel reference) framed in gold, surrounded by a
border of kaleidoscope wedges. The geometry barely moves — the color rolls through it.

## Math
- Panel cell (40 px): normalized u,v ∈ [−1,1], chirality flip `v ← v·(−1)^(gx+gy)`.
- Square spiral: `r = max(|u|,|v|)`, `a = atan2(v,u)/2π`, key where
  `frac(3r − a + 0.004·t) < 0.5` — a rectilinear spiral hook per cell.
- Key hue 0.02 vs ground hue 0.55, both + 0.0015·t (whole-palette rotation, per R2).
- Border: 8 wedges `frac(8θ/2π + 0.003·t)`, quantized to 4 hue steps; brightness
  ripples with panel-distance `max(|px|/120,|py|/80)`.

## Integer ARM64 plan
- Cell-local u,v by masking low bits (cell = 64 px in the real build → AND with 63).
- max(|u|,|v|) is pure integer; atan2 replaced by an octant-fold + small y/x LUT
  (or better: precompute ONE 64x64 key-mask tile at init from the same formula and
  blit it with per-cell chirality flip = reversed X reads — zero per-pixel math).
- Palette cycling = rotate DAC/palette LUT entries; the crawl term 0.004·t bakes into
  the tile regeneration once every few frames, not per pixel.
- Border wedges: quadrant fold + coarse angle LUT; frame test = two integer compares.

## Palette pairing
Two-family split: keys ride the warm end (red/orange), panel ground the cool end (blue),
border cycles cyan→violet. Whole-palette rotation swaps figure/ground over ~40 s,
recreating the yellow→red→blue recolor of frames a02→a03→a04.

## Motion
Palette rotation is the primary motion (0.0015 hue/frame ≈ 11 s per full family shift);
the key spirals crawl at 0.004 phase/frame — a barely-perceptible ooze; border wedges
precess opposite. Static-feeling, meditative, no strobe.

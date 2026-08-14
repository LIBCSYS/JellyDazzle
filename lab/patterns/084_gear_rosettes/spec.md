# 084 Gear Rosettes

## Look
Four large sunflower-gear rosettes in a 2x2 array — toothed golden discs with dotted seed
cores — sitting on a slow blue diagonal-stripe ground, with a chain of small cyan diamonds
running down the center vertical (straight from the R13 gear-flower reference, d22–d24).

## Math
- Gear edge: `R(θ) = 44 + 5·sin(16(θ + rot))`, rot = ±0.006·t + 0.7·i (alternating spin).
- Seed core: `seed = ½ + ½·sin(0.55·r − 0.012·t)·sin(8(θ − 2·rot))` — dot-ring
  interference of radial rings against 8 angular lobes.
- Rim glow: 1 − |r − R(θ)|/2.5 clamped.
- Ground stripes: sin(0.35·(x+y)/2 − 0.008·t); center column diamonds:
  manhattan distance of (|x−160|, mod(y + 0.05·t, 30) − 15).

## Integer ARM64 plan
- Per rosette bounding box only (~100x100 px each) — the stripe ground is a 1-D LUT
  swept diagonally (value depends on x+y only: one add per pixel + table read).
- r by octagonal norm; θ by octant-fold LUT; `sin(16θ)` = read angle-LUT at (θ<<4).
- seed = (sin16[r·k1 − t·k2] · sin16[8θ − rot2]) >> 15, both from ONE shared 16-bit
  sine table; compare r < R for fill, |r−R| < 3 for rim (integer band test).
- Figure/ground palette swap = DAC rotation of the blue↔yellow ramp.

## Palette pairing
Restricted blue/cyan/yellow/green family. Gears live on the yellow-gold arc of the
palette, ground on blue-cyan; the slow global cycle swaps figure/ground exactly like
d22→d24 (yellow-on-blue becomes blue-on-yellow).

## Motion
Gears rotate at 0.006 rad/frame in alternating directions (one revolution ≈ 17 s);
seed rings drift inward; diamond chain slides down at 0.05 px/frame; stripes creep.
Everything continuous, mechanical, unhurried.

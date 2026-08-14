# 026 Spoke Moire

## Look
Two fans of spiral spokes — nine arms against eleven — counter-rotate around the
center, and their angular beat sweeps a two-lobed brightness wave around a full
rainbow pinwheel while concentric rings breathe outward through it. Kin to the
original's smooth 6-arm pinwheel (R16), but born from pure interference.

## Math
Polar `a = atan2, r = |p-C|`. Spoke gratings with opposite spiral twist:
`g1 = sin(9a + .02r + .012t)`, `g2 = sin(11a - .016r - .010t)`.
Beat `f = (g1+g2)/2` — envelope `cos((2a + Δ)/…)` is a 2-lobe pinwheel rotating at
the difference rate. Rings `sin(.16r - .045t)`. Hue = `a/2π + 0.10·ring + drift`;
Val = `(0.5+f/2)(0.55+0.45·ring)`.

## Integer ARM64 plan
- Precompute per-pixel byte LUTs `ang[i]` (angle 0..255) and `rad[i]` once at init
  — the tunnel-effect machinery. Frame loop is pure table math.
- `g1` index = `(9*ang[i] + (rad[i]>>2)·k1 + p1) & 0x3FF` — small integer muls
  (9, 11 fit in a shift-add: 9x = x<<3 + x) + sine LUT reads. Rings = third LUT
  read on `rad`.
- Sum/scale in 16-bit lanes; final index into a 2D palette `pal[ang_band][val_band]`
  or hue wheel LUT of 256 entries rotated per frame.
- Zero atan2/sqrt at frame time; everything angle-ish is the precomputed byte LUT.

## Palette pairing
Full spectral wheel mapped around the center (hue = angle), locally bent by the
ring phase so color bands ripple outward. Angular beat modulates saturation and
value, not hue — so the rainbow stays cohesive while brightness lobes orbit.

## Motion
The two spoke systems turn opposite ways slower than 0.15 rpm each; their beat
envelope (the visible bright lobes) rotates at the difference frequency — slow and
stately. Rings drift outward ~0.7 px/frame. Constant flowing motion, zero strobe.

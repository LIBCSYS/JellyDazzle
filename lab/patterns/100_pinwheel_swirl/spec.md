# 100 Pinwheel Swirl

## Look
Six smooth-shaded comma arms spiral out of the center over black, turning slowly clockwise while the spiral's tightness breathes in and out; the shading ramp rolls from green-dominant through yellow rims toward red-dominant over the run. This is the big clean top-left tile of the f-series wide shots (R16) — the original's one true computed plasma field, dithered shading and all.

## Math
- Field: `v = cos(6θ + k(t)·r^0.95 − 0.012t)` with `k(t) = 0.085(1 + 0.22·sin(0.003t))` — the r-term bends the 6 angular lobes into commas; modulating k is the breathing.
- Shade: `((v+1)/2)^1.3 · edgefade(r)` + a STATIC ±0.035 dither field (ordered-dither look without temporal noise).
- Color: `hue = 0.34 − 0.30·shade + 0.00055t`, `sat = 1.15 − 0.35·shade`, `val = shade^0.85` → dark gaps stay black, arm cores go bright/whitish-yellow, and the whole wheel drifts green→red exactly like f01→f11.

## Integer ARM64 plan
Full-repaint field, one pass: θ from octant atan table (8-bit angle), r from octagonal norm; `r^0.95 ≈ r` is acceptable at VGA scale (or a 256-entry pow table on r>>1). Phase = `6·ang8<<8 + kQ·rT − t·kw` in 16-bit wrap, one sine-table read → shade byte; add the precomputed dither byte; final pixel = palette[shade]. The green→red hue roll is FREE: leave shade as the palette index and rotate/re-lerp the 256-entry DAC ramp each frame (the original provably did exactly this). Breathing k = 8.8 fixed-point from the sine table once per frame. Inner loop ≈ 6 integer ops + 2 table reads per pixel.

## Palette pairing
Single 256-level ramp black → deep green → green → yellow → warm white at the top, whose hue anchor slides toward red over ~1900 frames — reproducing f01 (green phase) through f11 (red phase) with yellow rims throughout. Black stays black; only the lit arms recolor.

## Motion
Rotation 0.012 rad/frame (~9 s per arm-to-arm step, ~52 s per revolution), tightness breath on a ~35 s cycle, hue revolution ~30 s. Three incommensurate slow clocks — the pattern never repeats exactly and never jumps. The flagship relaxer.

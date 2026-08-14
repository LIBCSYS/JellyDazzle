# 093 Cathedral Fan

## Look
A red/pink sunburst pinched flat at a glowing horizontal centerline, its rays sweeping like a slowly opening fan; blocky stair-stepped blue spires with green step-seams grow up and down from the horizon while golden concentric arcs breathe in the four corners. Replica of R3 (frames a05–a07 and the mid-left tile of the f-series wide shots).

## Math
- Mirrored quadrant: everything computed on `(|dx|, |dy|)` → free 4-fold symmetry.
- Rays: `ang = atan2(|dy|,|dx|)`; mask `frac(ang·30/(π/2) + t·0.006) < 0.42`; intensity `pinch·(0.35+0.65·r/170)` with `pinch = (1−ang/(π/2))^2.2` (dense/bright near horizontal); every 5th ray recolored gold.
- Horizon: `exp(−dy²/6)` warm white line.
- Spires at x-offsets {36,78,116}: width shrinks per 10-px stair step `w = w0 − step·k`; height grows with `clip(t/700 − phase)`; seams where `|dy| mod 10 < 2`.
- Corner arcs: `sin(dcorner·0.42 − t·0.03) > 0.55`, faded by distance.

## Integer ARM64 plan
Angle → octant-folded `atan` via 256-entry table on `min/max(ax,ay)` ratio (one 16-bit reciprocal-table multiply, no div). Ray mask is a byte compare on the fractional part of `angT·30 + t·k` in 8.8 fixed point; pinch² from a squared-byte table. Horizon/e^ falloff = 64-entry exp table on `ay²>>2`. Spires are pure rectangle fills per scanline (integer width per step — precompute widths). Corner arcs: `dcorner` via octagonal norm approx `max + 3/8·min`, ring test from the 16-bit sine table. All per-pixel work is table lookups and adds.

## Palette pairing
Black ground; rays crimson→pink→white by radius with sparse gold rays; spires deep blue with green seams; arcs amber — the red/white/green/gold family read directly off a05–a07.

## Motion
Fan sweeps at 0.006 rev-units/frame (one ray-width every ~70 frames); spires grow in over ~700 frames, each starting later than the last; arcs drift inward slowly; horizon is steady. A patient, cathedral-organ build — nothing pops.

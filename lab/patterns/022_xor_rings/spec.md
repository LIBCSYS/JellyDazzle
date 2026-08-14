# 022 XOR Rings

## Look
Two invisible ring emitters wander the screen and their bands XOR into crisp
lens-and-eye interference figures — hard-edged fringes stepped from midnight indigo
up through royal blue to molten gold. Reads like engraved metal that keeps re-etching
itself.

## Math
`d_i = round(1.6 |p - c_i| ± phase_i(t))` (integers), centers on slow Lissajous
orbits. Index `v = (d1 XOR d2) & 63`; the XOR of two ring sawtooths yields
hyperbola/ellipse fringe families. Color ramp on `u = v/63`:
hue `0.62 - 0.55 u^1.3`, val `0.14 + 0.86 u`, sat `0.92 - 0.35 u²`.

## Integer ARM64 plan
- The canonical 1990s effect, natively integer: one oversized distance byte table,
  two moving window offsets `o1,o2`; per pixel `idx = (dist[o1+i] + p1) EOR
  (dist[o2+i] - p2) AND 63` — two loads, add/sub, EOR, AND. NEON does 16 pixels
  per iteration with `LD1/EOR/AND/TBL`.
- Ring phase animation (`±0.3t`) is a per-frame byte constant added before the EOR.
- 64-entry palette LUT (indigo→gold ramp) rebuilt per frame for the slow hue drift;
  per-pixel color is a single `TBL` byte-table lookup into it.
- No trig, no sqrt, no division anywhere in the frame loop.

## Palette pairing
Duotone discipline like the original's red/blue machine (R7): indigo #1a1a5e floor
through cobalt to saturated gold #ffcc33 peaks, with the whole ramp hue-drifting
very slowly. High contrast but only 64 quantized steps — bold, never noisy.

## Motion
Centers drift at ~0.006 rad/frame; ring phases creep in opposite directions so
fringes continuously slide through each other like slow gears. Every part of the
figure is always moving but no band advances faster than ~1 px every 3 frames.

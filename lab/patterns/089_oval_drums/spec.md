# 089 Oval Drums

## Look
Full-screen concentric racetrack (stadium) rings in rolling rainbow bands radiate from a
pair of white-framed striped drums that sit side by side at center, their stripe ramps
rolling in opposite directions (the d28–d30 oval routine), with soft plum blobs pulsing
on the perimeter at mirrored positions.

## Math
- Stadium distance: `d = hypot(max(|x|−70, 0), y)` — distance to a horizontal segment;
  rings = band index ⌊d/15 − 0.008t⌋ (rings shimmer outward), band shading
  sin(π·frac), band hue = 0.13·index + 0.0012·t.
- Drums: rects |x∓38|<31, |y|<21; stripe phase `g = frac(y/12 − 0.016t·sx)`;
  drum hue ramp 0.32 − 0.30g (+ global cycle) = green→red rolling ramp.
- Blobs at mirrored (|x|,|y|) offsets, intensity 1 − d/10, pulsing sin(0.013t).

## Integer ARM64 plan
- Stadium d: per pixel it's |y| when |x|<70 (row constant band!) else octagonal norm
  of (|x|−70, y) — the middle strip is literally a row LUT; only the two end caps
  need norms. Ring index/frac via reciprocal-free: keep running d in Q8.8, subtract
  sp with a counter (Bresenham-style ring stepping along each row).
- Ring shading: 16-entry half-sine band profile LUT indexed by frac bits.
- Drums: rectangle fills; stripe value/hue from a 256-entry per-frame column LUT
  (depends on y only) — one table read per pixel.
- Palette cycling on ring hues + drum ramp via DAC rotation.

## Palette pairing
Rings walk the full rainbow at 0.13 hue/ring (adjacent rings clearly distinct, like the
original's acid multicolor bands); drums carry a green→red ramp that the global cycle
slides toward blue over ~30 s; outlines warm white; blobs plum.

## Motion
Rings shimmer outward one band every ~2 s; the two drums roll their stripes in opposite
directions (soothing counter-motion, full stripe cycle ~6.5 s); perimeter blobs breathe.
Slow palette rotation recolors everything continuously.

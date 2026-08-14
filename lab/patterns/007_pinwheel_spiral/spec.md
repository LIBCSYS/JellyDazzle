# 007 Pinwheel Spiral

## Look
A smooth-shaded log-spiral pinwheel — the direct heir of the original's R16 tile — whose arm count steps through 6, 9, 12, 8, 10 with soft crossfades, while spiral tightness breathes through zero so arms uncurl straight and recurl the other way. Gold, black, sage and cream, velvet-smooth shading.

## Math
- Field: a = N·theta + 3.4·sin(0.0015t)·ln(r+4) − 0.016t; v = 0.5 + 0.32·sin(a) + 0.16·sin(2a + 0.03r + 0.005t). Pure C_N rotational symmetry — deliberately no mirror, so the pinwheel has handedness.
- ln(r) makes arms logarithmic spirals (constant pitch angle); the sin(0.0015t) coefficient sweeps pitch −3.4..+3.4, flipping chirality smoothly.
- N schedule [6,9,12,8,10], 300 frames/stage, smoothstep crossfade in the last 28%.
- Color: cosine palette c=(1,.7,.4), d=(0,.15,.20) — unequal frequencies give gold/olive/plum bands; brightness tied to v, hue drifts with r.

## Integer ARM64 plan
- Per-pixel needs theta (BAM atan2 poly) and ln(r): precompute a 256-entry log2 LUT indexed by r>>1 (screen r ≤ 200) — one lookup replaces the float log; scale by the per-frame Q8.8 pitch coefficient with one multiply.
- a = N·theta is a BAM multiply (wraps free); v = two quarter-wave lookups + adds.
- Crossfade identical to 001 (two field evals, Q8 blend). Since both fields share theta and the log lookup, marginal cost of the second field is 2 multiplies + 2 lookups.
- Radial hue bias: add (r·k)>>shift into the palette index; palette itself is the 256-entry DAC, rebuilt per frame from the cosine coefficients.

## Palette pairing
Black-cored gold arms with sage-green and plum inter-arm glow — an autumn-brass scheme; the dark core anchors the rotation so motion reads without glare.

## Motion
Arms revolve at 0.016 rad/frame ÷ N (~1 rev / 40 s at N=6); pitch breathes over ~70 s including two graceful chirality flips; N changes every 5 s behind a 1.4 s fade. The slowest, smoothest pattern of the set.

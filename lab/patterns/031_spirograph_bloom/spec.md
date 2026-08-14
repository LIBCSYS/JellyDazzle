# 031 Spirograph Bloom

## Look
A luminous spirograph rosette fills the screen, its petal count and loop depth slowly breathing as the gear ratio drifts, the whole figure cycling through saturated single-hue schemes (chartreuse to cyan to magenta) over a couple of minutes. Older strokes fade gently so the bloom is always rebuilding itself out of its own afterglow.

## Math
Hypotrochoid, drawn as a persistent accumulation with exponential age fade:
- theta advances 0.11 rad/frame (36 substeps/frame)
- x = 0.60·R·cos(theta) + R·d(n)·cos(q(n)·theta), y = 0.60·R·sin(theta) − R·d(n)·sin(q(n)·theta), R = 92
- q(n) = 2.5 + 1.2·sin(0.0021·n)  (winding ratio drift)
- d(n) = 0.35 + 0.25·sin(0.0009·n + 1.7)  (pen offset breathe)
- stroke weight w = exp(−age/380); dihedral D6: 6 rotations × y-mirror = 12 copies
- hue = 0.0028·theta + 0.0007·t (slow scheme cycling), S=0.95

## Integer ARM64 plan
- theta as BAM16 phase accumulator, increment 0x0480-ish per substep; q and d from the 16-bit quarter-sine table driven by two more slow BAM accumulators (increments 0x000E, 0x0006 in 32-bit sub-BAM).
- q·theta: Q8.8 ratio times BAM angle, product >>8, natural wraparound.
- Point = 4 sine lookups + 3 Q15 multiplies. Plot into a 320x240x3 16-bit accumulation buffer.
- 12-fold symmetry: precompute 6 (cos,sin) sector pairs (Q15); each point deposited 12× via one 2×2 fixed-point rotate + y-negate — no per-pixel work at all.
- Fade: once per frame multiply whole accumulator by k = 65364/65536 (≈ exp(−1/380)) — one 16-bit mulhi per cell, or decay every 4th frame with a stronger k to save bandwidth.
- Tone map accumulator→palette through a 256-entry gamma LUT; no per-pixel float, trig, or division anywhere.

## Palette pairing
Single saturated hue on near-black violet vignette; hue index crawls ~1 full cycle per ~1400 frames so any single screen reads as one cohesive scheme. Pair with a 256-entry HSV-ring palette rotated by frame counter (classic palette-cycling trick — zero redraw cost).

## Motion
Nothing translates; the figure slowly re-threads itself as the gear ratio drifts (full morph cycle ≈ 50 s at 60 fps), while the global hue drifts a degree or two per second. Fade constant 380 frames keeps ~6 s of trail — smooth bloom, zero strobe.

# 056 Ribbon Swarm

## Look
Three hundred particles strung along a phase gradient of a 2:3 Lissajous form a
single rainbow ribbon that folds, twists and unfolds like a starling murmuration;
left/right mirroring (plus a ghost vertical reflection) turns it into a woven bow-tie
figure on indigo.

## Math
Particle i (u = i/N): phase `ph = 2π·u·spread(t)`, `spread = 1.7 + 1.1 sin(0.0019t)`.
Position: `x = 118 sin(2θ + ph)`, `y = 86 sin(3θ + 1.5ph + 0.0031t)`, `θ = 0.0095t`.
Short trail: re-evaluate at `t - 2.2j`, j = 0..8, weight `(1-j/9)^1.4`. Hue = 0.9u +
slow drift; mirror across x (full weight) and y (18% ghost).

## Integer ARM64 plan
Pure sine-table pattern — every coordinate is `A·sin(k·θ + ph_i)` with 16-bit table
lookups; `ph_i` per particle is a precomputed fixed-point array scaled each frame by
one multiply (`spread` from a second slow table walk). 300×9 evaluations ≈ 2.7k
points/frame, each 2 lookups + 2 multiplies; mirrors are negations around the center.
Full repaint per frame (fast clear via DMA/STP zero fill) — no state at all.

## Palette pairing
Hue ramp 0→0.9 along the ribbon (rainbow snake, head-to-tail); indigo ground;
whole ramp drifts one cycle / 28 s. Head samples desaturate slightly for sheen.

## Motion
The ribbon's fold parameter breathes over ~55 s; the underlying Lissajous turns
continuously (~11 s per lobe cycle); trails are short so motion reads as flight,
not smear. Slow, dreamy, endless.

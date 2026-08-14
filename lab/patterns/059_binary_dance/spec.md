# 059 Binary Dance

## Look
A gold sun and a cyan sun waltz around their barycenter dragging interlocking spiral
trails; each carries four counter-rotating moons with their own short tails, and a
gauzy figure-8 particle stream flows between the two, blending gold into cyan across
the crossing. Twinkling star dust behind.

## Math
Suns: `pos = C + R(cos(Ωt+σ), 0.78 sin(Ωt+σ))`, `Ω = 0.0045`, `σ ∈ {0, π}`; trail =
140 samples at `t-2j`. Moon m: radius `13+4.8m`, rate `±(0.052-0.007m)` about the
(moving) parent — trails re-evaluate parent position at past times, so tails curve.
Stream: lemniscate of Bernoulli `x = L cos s/(1+sin²s)`, `y = 1.35 L sin s cos s/(1+sin²s)`
rotated by Ωt, particles at `s = 0.008t + 2πk/55`; hue lerps gold↔cyan by `cos s`.

## Integer ARM64 plan
Sine table gives every angle. Lemniscate divide: `1/(1+sin²s)` is a function of one
table angle → bake a 256-entry 0.16 fixed reciprocal table, one lookup + two
multiplies per stream dot. Moon-around-moving-parent trails come free from ring
buffers of plotted coords (no re-evaluation). Total live math/frame: 2 suns + 8
moons + 110 stream dots ≈ 260 table-multiply pairs. Trail fade via per-object
palette ramps in the indexed buffer.

## Palette pairing
Duotone-plus: gold ramp (hue 0.09) vs cyan ramp (0.55) — a deliberate restricted
scheme like the original's red/blue machine (c01–c05) — with the stream sweeping the
in-between hues so the two families visibly mix. Near-black blue ground, white dust.

## Motion
One waltz revolution ≈ 46 s; moons orbit 4–12 s each, alternating direction; stream
flows continuously through both lobes. Stately, hypnotic, zero flicker.

# 037 Pendulum Web

## Look
String art strung between two invisible Lissajous curves: every few ticks a rung is stretched from a point on curve A to the matching point on curve B, each rung a cyan-to-magenta gradient, building a taut woven scarf of light with 180-degree rotational symmetry. The counter-precessing curves slowly wring the web into new drapes.

## Math
Rungs between two parametric curves, accumulated with age fade:
- rung m: P1 = (105·sin(2s + φ), 80·sin(3s)), P2 = (105·sin(3s + 1.2 − φ), 80·sin(5s + 0.5))
- s = 0.213·m, φ = 0.0009·m  (curves precess in opposite directions)
- 3 rungs/frame, 40 samples each; weight w = exp(−age/800)
- copies: identity + 180° rotation
- hue = 0.55 + 0.35·u + 0.0004·t  (u = position along rung: cyan end → magenta end)

## Integer ARM64 plan
- Curve endpoints: 4 BAM16 phase accumulators per curve family (2s, 3s, 3s, 5s all share one base accumulator scaled by integer ratios — accumulate 4 increments, no multiplies), + sub-BAM φ drift.
- Endpoints = 4 sine lookups + 4 Q15 multiplies per rung pair.
- Rung: fixed-point DDA, one division per rung (or reciprocal table for constant S=40: dx = (x2−x1)·1638 >> 16), then 40 add-steps.
- Gradient along rung: palette index advances a constant step per DDA step — color is literally a counter.
- 180° copy: negate both offsets, second store.

## Palette pairing
Two-tone gradient per rung, cyan (H .55) to magenta (H .90), the whole ramp drifting around the wheel over ~4 min; near-black purple vignette. Gradient-per-rung means crossings blend into rich secondary tones instead of grey.

## Motion
Web wrings itself: counter-precession full cycle ≈ 2 min. Rungs arrive 3/frame with 13-second trails — the fabric look comes from persistence; per-frame change is a whisper.

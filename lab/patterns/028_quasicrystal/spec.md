# 028 Quasicrystal

## Look
Seven plane waves at seventh-turn angles superpose into an aperiodic quasicrystal —
a lattice of seven-fold rosettes that never exactly repeats, shimmering in peacock
blues, teals and violets. Alternate waves drift in opposite directions, so rosettes
continuously assemble, dissolve and re-assemble like standing waves on silk.

## Math
`f = (1/7) Σ_{i=0..6} cos(k·(x cosθ_i + y sinθ_i) ± 0.028t)`, `θ_i = rot + iπ/7`,
`k = 0.30`, whole wave-star rotating at `rot = 0.0012t`; odd waves take the minus
phase sign. Contrast shaping `g = tanh(2.6 f)` gives bold plateaus with thin walls.
Hue = `0.52 + 0.17 g`, Val = `0.14 + 0.86(0.5 + g/2)`.

## Integer ARM64 plan
- Each plane-wave argument is affine in (x,y): seven 16.16 DDA accumulators with
  per-row/per-pixel deltas computed once per frame from the sine LUT. Inner loop:
  7 adds + 7 sine-LUT reads (high byte of each accumulator), summed in 16-bit
  NEON lanes with `SQADD`.
- tanh shaping baked into a 256-byte remap table on the summed high byte (the
  classic clamp/response LUT) — zero per-pixel float.
- Final color: 256-entry peacock ramp `TBL` lookup; slow drift = ramp rotation.
- Rotation of the wave star touches only the 14 per-frame delta constants.

## Palette pairing
Peacock family: ink-navy walls, teal and cerulean plateaus, violet transition
bands; saturation eases off at extremes so peaks read as pearl rather than neon.
Slow whole-field hue drift keeps successive minutes differently keyed.

## Motion
Counter-drifting wave phases (±0.028 rad/frame) make each rosette pulse open and
closed over ~7 s while the entire lattice revolves once per ~87 s. Motion is
everywhere but glacial — pure standing-wave breathing, zero strobe.

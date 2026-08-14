# 054 Starfield Warp

## Look
A gentle hyperspace cruise: 520 stars stream outward from a vanishing point that
itself drifts on a slow Lissajous, leaving short radial streaks whose hue is set by
their angle — the whole field reads as a slowly turning color wheel of stars over two
breathing nebula clouds (rose and teal).

## Math
Star: unit-plane pos `(u,v)` gaussian, depth `z = ((z0 - 0.0022 t) mod 1.1) + 0.07`.
Projection `px = cx(t) + u·F/z`, `F = 170`; streak = 7 samples between z and
`z + 3·0.0022`. Brightness `∝ 1/z` (capped), saturation `∝ z` (near stars whiten).
Hue `= atan2(v,u)/2π + 0.0005t + 0.12 z0`. Center `cx,cy` wander ±16 px at ~0.004 rad/f.
Nebulae: two gaussians drifting sinusoidally.

## Integer ARM64 plan
The killer float here is `1/z`. Quantize depth to 256 steps and use a 256×16-bit
reciprocal table → projection is `(u * recip[z]) >> 8` per axis. Depth update is an
integer subtract with wraparound mask. Streaks: remember last frame's projected point
per star (2×16-bit) and draw a 3-step Bresenham segment — no second projection.
Angle-hue: precompute each star's palette index at init (atan2 once on the host or a
CORDIC pass), then only the global palette rotates. Nebulae are static bitmaps whose
palette entries breathe.

## Palette pairing
Full 360° hue wheel at moderate saturation for stars (white-hot near ones), over
rose/indigo + teal nebula ramps. Wheel rotates ~1 cycle / 35 s.

## Motion
Steady 8-second star transit, drifting vanishing point (no fixed tunnel), soft
twinkle at 0.05 rad/frame. Everything glides; nothing pops except star rebirth at
the far plane, which is dim by construction.

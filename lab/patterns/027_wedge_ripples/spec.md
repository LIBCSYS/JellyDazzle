# 027 Wedge Ripples

## Look
An eight-petal jade-and-emerald mandala: two wave sources ripple inside a single
kaleidoscope wedge and the fold mirrors them into sixteen phantom emitters whose
fringes kiss along every seam, forming lens-shaped "eyes" on the petal boundaries.
The whole rosette slowly revolves while the ripples crawl through it.

## Math
Fold: `fa = |((a + .0035t) mod (2π/8)) - π/8|`, wedge coords `u = r cos fa`,
`v = r sin fa`. Two sources orbit inside the wedge (off the fold axis, per the
research caveat): `f = sin(.30 d1 - .04t) + sin(.26 d2 + .03t)`.
Hue = `0.34 + 0.07f + 0.10 sin(.012r - .01t)`; Val adds an envelope term
`cos²(0.075(d1-d2))` for bright interference eyes.

## Integer ARM64 plan
- The entire fold is a precomputed `fold_map[76800]` of source offsets into a
  wedge buffer (the universal demoscene fold LUT). Rebuild is NOT needed per
  frame: fold rotation is done by adding a phase to the angle byte before the
  wedge lookup — angle LUT + add + AND, still init-time tables only.
- Render the ripple field only into the wedge buffer (~1/8 of the pixels):
  two distance-table reads + two sine-LUT reads per wedge pixel, then
  `screen[i] = wedgebuf[fold_map[i]]` — one indirection for 7/8 of the screen.
- Wedge-space source orbits are 4 sine-LUT evals per frame.
- Palette via 256-entry jade ramp, `TBL` lookup; drift by ramp rotation.

## Palette pairing
Jade/emerald body (hue 0.28-0.44) with radial hue waves adding teal and spring
green; interference eyes bloom toward pale gold-white via the value envelope.
One cohesive cool-green scheme with warm accents only at constructive peaks.

## Motion
Rosette spin: one revolution ≈ 30 s. Sources orbit at 0.005-0.009 rad/frame so
petal interiors churn slowly; ripple phase crawls at 0.03-0.04 rad/frame.
Everything is phase-continuous — the mandala turns, breathes, never blinks.

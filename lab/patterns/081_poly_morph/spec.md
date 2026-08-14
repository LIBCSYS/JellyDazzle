# 081 Poly Morph

## Look
Five glowing medallions — one large center, four mirrored satellites — float over a dim violet
ring-field; each medallion is a filled regular polygon that slowly melts from triangle to square
to pentagon to hexagon and back, with a bright rim and a rainbow-shaded interior.

## Math
- n-gon radius at angle θ: `R(θ,n) = cos(π/n) / cos((θ mod 2π/n) − π/n)`.
- Morph: side count `s(t) = 3 + (0.0035·t + φᵢ) mod 4`; blend the two integer radius
  functions with smoothstep `m = 3m²−2m³` of frac(s).
- Inside test: `d = r / (R·R(θ,n))`, inside when d < 1; rim = 1 − 12·|d−1| clamped.
- Interior hue = d·0.9 + 0.19·i + 0.0012·t through a cosine rainbow palette.
- Ground: 0.10 + 0.06·sin(0.05·r₀ − 0.01·t) on violet.

## Integer ARM64 plan
- Per medallion, precompute a 256-entry Q8.8 table of `cos(π/n)/cos(a)` per side-count pair,
  blended once per frame (the morph blend is per-frame scalar work, not per-pixel).
- θ via octagonal atan2 approximation (CORDIC-lite or table on y/x quadrant-folded);
  r via alpha-max-beta-min octagonal norm — no per-pixel sqrt/trig.
- d = r · recip_table[R·rr] with one 16-bit reciprocal table lookup; inside/rim from
  integer compares. Hue index = (d·230 + i·49 + t·0.3) into a 256-entry palette LUT.
- Only 5 objects: bounding-box the medallions, paint ground with a ring LUT first.

## Palette pairing
Cosine rainbow (phase 0/.33/.67) for medallions over desaturated violet ground —
objects read hot, field reads cool; rims near-white blue.

## Motion
Satellites spin ±0.004 rad/frame (alternating direction), side-count morph completes a
full 3→6→3 cycle in ~19 s at 60 fps, ground rings drift inward at 0.01 phase/frame.
Nothing strobes; the morph is the star.

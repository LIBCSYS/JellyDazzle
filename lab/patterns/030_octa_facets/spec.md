# 030 Octa Facets

## Look
Two trains of concentric octagon rings glide from a pair of wandering centers and
interfere into a faceted crystal lattice — bright ice-white ridge lines over deep
blue and violet cells, like frost ferns growing on cut glass. Edges are straight
and jewel-like (octagonal norm), not round: pure "dazzle" geometry.

## Math
Octagon distance `d8(p) = max(|dx|, |dy|, (|dx|+|dy|)/√2)`.
Ring trains as triangle waves `r_i = tri(0.055 d8_i ∓ phase_i(t))`,
`tri(z) = |z mod 2 - 1|`. Interference `f = r1 + r2 ∈ [0,2]`; ridge lines
`ridge = tri(f)` peak where trains agree.
Hue = `0.60 + 0.15(f-1)`; Val = `0.14 + 0.86 ridge^1.3`; Sat rises off-ridge.

## Integer ARM64 plan
- The centerpiece octagonal norm is the classic integer distance approximation:
  `max(max(|dx|,|dy|), (|dx|+|dy|)*181 >> 8)` — abs, max, add, mul-by-constant,
  shift. NEON `SABD/SMAX/USHR` across 8 lanes; no sqrt ever.
- Triangle wave = XOR-fold on the low bits: `v = d & 0x1FF; v = v ^ ((v<<23)>>31
  sign trick)` or simply `tri = min(v, 512-v) >> 1` — pure ALU, no sine table
  even needed.
- Sum r1+r2 (bytes), ridge = same triangle fold again. Final color = 256-entry
  ice/violet palette `TBL` on the ridge byte + a hue offset from the f byte via
  a second small LUT.
- Ring phase animation = per-frame byte constants; center motion = 4 sine LUT
  evals per frame.

## Palette pairing
Ice family: near-black indigo cell floors, cobalt and periwinkle mids, violet
where one train dominates, ridge lines blooming to glacial white-cyan.
Saturation is lowest exactly on ridges so they read as light, not neon.

## Motion
Centers wander on slow Lissajous paths (~0.005 rad/frame); one ring train drifts
outward, the other inward, so facet cells continuously split and merge like slow
cell division. Ridges migrate ~0.5 px/frame. Crystalline but calm — no strobe.

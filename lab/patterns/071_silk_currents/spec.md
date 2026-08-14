# 071 Silk Currents

## Look
Hundreds of silk threads drift along invisible currents, braiding into glowing violet-to-green rivers over a deep indigo vignette. Trail heads burn bright while tails fade to smoke, so the whole field looks like luminous hair combed by slow wind.

## Math
- Angle field: `a(x,y,t) = 1.15·[sin(0.021x+φ) + sin(0.017y−0.7φ) + sin(0.011(x+y)+0.4φ) + sin(0.02·dist(c)−0.5φ)]`, φ = 0.007t.
- 800 particles, start points on slow Lissajous orbits of their home cell; 110 integration steps of `(x,y) += 1.6·(cos a, sin a)`.
- Deposit weight `w = 0.05 + 0.55·(s/S)²` (head-bright), additive; tone map `1−exp(−k·acc)`.
- Hue = 0.52 + 0.45·hash(particle) + 0.0006t.

## Integer ARM64 plan
- Angle field on a 20×15 coarse grid of u8 angles, rebuilt once/frame from 4 sintab reads per cell — the plasma loop *is* the field (research §1).
- Particles in Q8: `x += costab[a]·409 >> 8` (1.6 in Q8). No float, no div.
- Distance term via octagonal norm `max+min/2−min/8` feeding sintab.
- Trails: persistent accumulate buffer with decay `v −= v>>5` per frame instead of re-integrating 110 steps — cheaper and identical look; head brightness is just fresh deposits being brightest.
- Hue → 256-entry palette ramp indexed by particle id + frame counter.

## Palette pairing
Analogous sweep cyan→violet→magenta (H 0.52–0.97) rotating 1 wheel / ~28 min; near-black indigo ground `(8,3,26)` with subtle center lift.

## Motion
The field phase drifts (full cycle ≈ 15 min) so rivers slowly re-route; particle homes orbit a few px; global hue rotates imperceptibly. No element moves faster than ~2 px/frame; nothing blinks.

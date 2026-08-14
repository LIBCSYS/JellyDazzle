# 049 Voronoi Breath

## Look
Stained glass alive: jewel-toned Worley cells — emerald, violet, ruby, gold — separated by dark leading that slithers as nineteen seed points orbit in three symmetric rings. Cells swell, get swallowed, and re-emerge as the rings breathe through each other.

## Math
- Seeds: origin + rings of 6 at radii 45/95/145 (±14 px sinusoidal breathing, per-ring phase), ring angular speeds +0.0016/−0.0011/+0.0008 rad/frame (counter-rotating).
- Per pixel: distances to all 19 seeds → F1 (nearest), F2 (second), idx (owner).
- Cell color: hue = frac(idx·0.381966 + 0.0004·t) — golden-ratio spacing → neighbors never share a hue; cospal(hue).
- Walls: brightness × (0.22 + 0.78·smoothstep((F2−F1)/10)) — dark where F2≈F1.
- Seed glow: × (0.80 + 0.20·exp(−F1²/800)).

## Integer ARM64 plan
- Repaint, research §8 method 1: track min and second-min SQUARED distance (dx²+dy² in i32 — never sqrt); 19 candidates/pixel is fine (mul cheap on Apple Silicon), or bucket seeds 3×3-grid style to cut to ~6.
- Wall term needs F2−F1 of true distance; use squared-distance difference normalized by a reciprocal LUT, or take the octagonal norm instead of L2 throughout (crystalline cells — a legitimate style variant, research §8 method 2).
- Owner hue: idx·105 (golden ratio × 256, u8 wrap) + t drift → palette LUT. Glow: u8 falloff table on F1.
- Seed positions: sintab-driven, computed scalar once per frame.

## Palette pairing
Muted-rainbow cosine palette (a=b≈0.44, d=(0,1/3,2/3)): saturated jewel tones with black leading — literal stained glass. Whole-window hue drift, one cycle ≈ 100 s.

## Motion
Ring orbits take 65–130 s per revolution; radial breathing ~21–35 s periods, staggered, so cell topology changes every few seconds — one wall pinches off, another opens — but each individual edge moves under 1 px/frame. Deeply calm.

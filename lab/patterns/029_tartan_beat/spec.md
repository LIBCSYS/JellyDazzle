# 029 Tartan Beat

## Look
Two woven square lattices — one turning clockwise, one counter, at slightly
different scales — interfere into a giant slow moire plaid: broad diagonal bands
of jade sliding against copper, with a fine glittering weave running through the
cloth. Like watching two window screens breathe against each other in stained
glass colors.

## Math
Grid field `G(ang,f,ph) = cos(f u + ph) + cos(f v - ph)` in rotated coords.
`g1 = G(0.16 sin(.0025t), 0.36, .02t)`,
`g2 = G(-0.14 sin(.002t) + 0.10, 0.395(1+.05 sin .003t), -.017t)` —
BOUNDED angle swings keep the relative angle small so moire cells stay large.
Moire `f = g1 + g2`; weave `w = g1 g2 / 4`.
Hue = `0.235 + 0.175 tanh(0.9 f)` — copper↔jade split;
Val = `0.14 + 0.72(|f|/4)^1.1 + 0.20|w|` (both color bands glow, gaps go dark).

## Integer ARM64 plan
- Each rotated (u,v) pair is affine → DDA accumulators: 4 accumulators total
  (u1,v1,u2,v2), 4 adds per pixel, deltas from the sine LUT once per frame.
- 4 cosine-LUT reads (cos = sin table with +90° offset), summed 16-bit; weave
  product = one `SQDMULH`.
- tanh hue split + value response collapse into one 2D palette LUT
  `pal[f_band][w_band]` (32x8) rebuilt per frame — per pixel one `TBL` read.
- Scale breathing of grid 2 only alters that frame's delta constants.

## Palette pairing
Copper (hue ~0.07, warm amber-orange) against jade (hue ~0.38) with the tanh
keeping mid-band transitions olive-gold rather than muddy; weave sparkle briefly
desaturates toward cream at thread crossings. Two-family scheme, high cohesion.

## Motion
Lattices counter-rotate at under 0.01 rpm; the moire plaid (the visible structure)
amplifies that into stately diagonal band drift. Grid-2 scale breathes over ~35 s
so the plaid period slowly swells and tightens. All drift, no flash.

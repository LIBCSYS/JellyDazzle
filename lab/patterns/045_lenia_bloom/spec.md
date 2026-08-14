# 045 Lenia Bloom

## Look
A colony of glowing ring-shaped "orbium" cells — cream, sage, and cyan on near-black — orbiting in three concentric symmetric rings. As the rings breathe, neighboring cells overlap and their membranes fuse into amoebic super-cells, then pull apart again.

## Math
- Cell = Gaussian ring bump: g(d) = exp(−(d−r₀)²/2w²) around a moving center.
- Layout: 1 breathing central cell + rings of 6, 6, 12 cells at radii 62/88/128 (± up to 18 px sinusoidal wobble, per-ring phase), ring radii 16/24/17 px. All cells in a ring share phase → 6-fold symmetry preserved.
- Connective halo: +0.35·g(R; 96±14, 34).
- Field L = tanh(1.25·Σ) — the tanh is what merges overlapping membranes into one smooth blob (the Lenia look).
- Color = cospal(0.62·L + 0.0004·t) · (0.10 + 0.90·L) — dark background, luminous cells.

## Integer ARM64 plan
- 25 cells/frame: per cell, per pixel: d via quarter-plane radius LUT or octagonal norm (never sqrt); ring profile exp(...) replaced by a 256-entry u8 bump table indexed by |d−r₀| (clamped) — one subtract, one abs, one table read, one add into a u16 accumulator.
- tanh replaced by a saturating response LUT on the accumulator (u16→u8).
- Cell centers: sintab-driven (angle = base + t·spd; radius wobble = sintab) computed once per frame, scalar.
- Optional: render field at half res, 2× upscale — the pattern is inherently soft.

## Palette pairing
Cosine palette d=(0.55,0.35,0.20) over a black floor: bioluminescent oranges → sage → cyan as L rises; global drift slowly re-tints the whole colony (deep-sea to lagoon over ~80 s).

## Motion
Ring orbits: ~48 s to ~90 s per revolution, inner and mid rings counter-rotating. Radial breathing on 3 incommensurate periods (~14/20/17 s) so merge events never repeat exactly. Merging/splitting membranes are the show; nothing moves faster than ~2 px/frame.

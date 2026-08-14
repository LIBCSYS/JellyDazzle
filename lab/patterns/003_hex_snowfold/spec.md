# 003 Hex Snowfold

## Look
A p6m-flavored hexagonal wallpaper: every cell of a triangular lattice holds a 12-petal snowflake rosette built by a D6 mirror fold, tiled edge to edge like stained glass. The whole field drifts while each rosette slowly spins in place and the palette walks from gold/blue to red/cyan.

## Math
- Lattice reduction: skewed coords u = x − y/√3, v = 2y/√3; delta to nearest lattice corner du = u − L·round(u/L), dv likewise; back to cartesian dx = du + dv/2, dy = dv·(√3/2). Corners are the 6-fold rotocenters.
- D6 fold about the rotocenter: w = pi/6; theta' = |((theta + 0.0018t) mod 2w) − w|.
- Source: v = 0.5 + 0.30·sin(0.16r − 0.010t + 4·theta') + 0.20·sin(0.30r·cos(6·theta') + 0.006t) — petal rings plus branch stripes; hue biased by r so rosette rims differ from cores.
- L = 72; lattice drift (0.11, 0.05) px/frame.

## Integer ARM64 plan
- Skew transform: two Q15 multiplies with constants 18919 (1/√3) and 37837 (2/√3) (geometry.md §2 p6m). round(u/L) → add L/2, multiply by precomputed reciprocal (mulhi), shift — no division.
- D6 fold: the fixed-N=6 reflection sequence from geometry.md §1 (y=|y|, two conditional Refl60 with 16384/28378, one Refl30) — bounded at 3 reflections, no atan needed at all; or the BAM fold if the local spin is kept.
- r from alpha-max-beta-min; petal terms are two quarter-wave table lookups; cos(6·theta') via BAM multiply-then-lookup.
- Per-cell continuity note: nearest-corner switching keeps r continuous at Voronoi boundaries (equidistant), so only mild angular seams — hidden by keeping the angular amplitude ≤ radial amplitude, same trick as the float proto.

## Palette pairing
Gold/olive rosettes on royal blue shifting to scarlet on cyan — high-saturation stained-glass primaries, closest sibling to the original's R18 hex mosaic tile.

## Motion
Field drift ~0.12 px/frame, per-rosette spin 0.0018 rad/frame (~1 rev/min), ring waves crawl outward at 0.010/frame, palette walks at 0.0004/frame. Dense but calm.

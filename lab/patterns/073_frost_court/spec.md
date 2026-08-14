# 073 Frost Court

## Look
A six-fold snowflake of frost dendrites grows outward from a hexagonal seed plate on a midnight-navy pane, arms feathering into cyan-teal side spurs that whiten near the core. Shimmer runs along the arms like light crawling on ice while the whole crystal rotates almost imperceptibly.

## Math
- One arm along +x: spine 0..L, `L = min(105, 0.16(t+250))`; side branches at every 8 px at ±60°, length `min(0.42·(108−node), 0.85·(L−node))`; sub-spurs every 10 px on branches.
- Whole arm replicated by 6 rotations (`+ t·0.0012` global spin); hexagon plate `r = h/cos((θ mod 60°)−30°)`, h grows to 13.
- Shimmer `v = 0.62+0.38·sin(0.22s − 0.045t)`; saturation rises with radius (white core → cyan tips).

## Integer ARM64 plan
- Branch geometry is fixed offsets: precompute the arm's point list once per growth step (integer node table); only the *count* of visible points changes with t — store points sorted by birth distance and draw prefix `[0, L(t))`.
- 60° rotations via sintab pairs; 6 rotations reuse the same point list.
- Shimmer = palette-index offset `(s·56 − 11t) & 255` into a 256-entry ice ramp — no per-point multiply.
- Accumulate buffer optional; pattern is cheap enough to repaint (≤ ~4K points).

## Palette pairing
Ice ramp: white core → pale cyan → teal (H 0.50–0.58, S 0.1→0.8); ground deep navy radial `(3,5,18)→(8,18,50)` glow. Single-hue family on purpose — reads as crystal.

## Motion
Growth ~1 px per 6 frames; rotation one revolution ≈ 87 min; shimmer wave travels arm-to-tip in ~20 s. When full, the flake holds, shimmers, then melts (fade) and reseeds with new branch spacing.

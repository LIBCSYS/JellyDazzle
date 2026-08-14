# 090 Diamond Burst

## Look
One huge diamond dominates the screen (the a01 rhombus-kaleidoscope): a magenta-rimmed
confetti-mosaic heart wrapped in expanding concentric diamond bands that cycle
cyan/magenta/green, on near-black, with soft cyan streaks escaping the left/right vertices.

## Math
- Diamond norm `m = |x| + 1.45|y|`; outer silhouette at D=118, inner heart at 0.62D.
- Rim bands: phase = m/16 − 0.010t; band index ⌊phase⌋ → hue 0.31·i + cycle;
  band profile sin²(π·frac) — bands are born at center and expand outward.
- Confetti: 4-fold mirrored cell coords (|x|,|y|)/8, per-cell hash h; shimmer
  `½+½·sin(2πh + 0.02t + 0.4(ix+iy))`; hue = 0.85h + 0.0015t.
- Streaks: exp(−|y|/5)·(0.4 + 0.3·sin(0.11x − 0.014t)).
- Edge glows: 1 − |m − D|/2.2 (cyan) and at 0.62D (magenta).

## Integer ARM64 plan
- m = |x| + |y| + (|y| >> 1) approximates the 1.45 factor with adds/shifts only;
  per scanline m changes by ±1 (±1.45 across rows) — walk incrementally.
- Ring index/frac by repeated-subtract stepping of sp=16 (frac = low 4 bits).
- Confetti: mirrored cell index = (|x|>>3, |y|>>3); hash = xorshift of packed indices;
  shimmer = sine-table read at (h<<8) + t·k — 8x8 blocks mean one computation per
  block, blitted as spans. 4-fold mirror: render one quadrant, flip-blit three.
- exp streak: 32-entry falloff LUT on |y|; sine on x from the shared table.
- Edge glow: two band compares on m.

## Palette pairing
Interior confetti spans the full rainbow (hash-anchored per cell) kept dim-to-bright by
shimmer; rim bands rotate a cyan→magenta→green triad (0.31 hue step ≈ 3-color cycle,
matching a01's rims); background near-black with sea-cyan streaks. Hot heart, cold void.

## Motion
Rim bands expand one spacing every ~2.7 s (outward birth at center, like a slow zoom);
confetti cells shimmer on ~5 s offset cycles (a quiet churn, never a strobe); streaks
ripple outward horizontally; the whole palette drifts around the wheel in ~14 s.

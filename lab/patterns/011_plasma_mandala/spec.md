# 011 Plasma Mandala

## Look
The canonical four-sine demoscene plasma folded six ways, so its blobs become breathing petals around a slowly turning center. Full rainbow hue flows through the petals while brightness swells and fades like slow surf.

## Math
- Fold: `w = 2π/6; fa = a mod w; fa = w−fa if fa > w/2`, with `a = atan2(dy,dx) + 0.004t` (whole mandala rotates).
- Wedge coords: `u = r·cos(fa)`, `v = r·sin(fa)`.
- Plasma sum: `c = sin(0.046u + 0.021t) + sin(0.071v − 0.017t) + sin(0.038(u+v) + 0.013t) + sin(0.052r − 0.019t)`.
- Color: HSV with `hue = 0.14c + 0.0009t`, `val = 0.46 + 0.36·cos(1.9c − 0.011t) + 0.16·cos(0.03r − 0.008t)`, `sat = 0.78 + 0.22·sin(0.9c + 0.006t)`.

## Integer ARM64 plan
- Precompute at init: per-pixel `angle_tab` (byte, 0–255 = full turn) and `radius_tab` (byte); fold the angle byte with masks (6-fold via a 256-entry fold LUT since 6 isn't a power of two).
- Precompute a `fold_map[76800]` of (u,v) packed source offsets so the frame loop never does trig.
- Plasma = the classic phase-buffer trick: two precomputed spatial sine-sum byte buffers `p1`, `p2` (built once over the folded (u,v) grid); per frame `c = p1[i+off1] + p2[i+off2]` with offsets stepped from `sin_tab[1024]` (1.15 fixed point). Radial term is `sin_tab[(radius_tab[i]·k + t4) & 1023]` — one more table read.
- Color via a 256-entry palette LUT regenerated per frame (or every 2 frames) from the HSV formulas in fixed point — per-pixel cost stays at 3 table reads + 2 adds. No per-pixel float, trig, or division.

## Palette pairing
Full HSV wheel at high saturation, value shaped by the plasma so petals have dark separations — reads as stained glass, never washed out. Hue base drifts one full cycle every ~18 minutes.

## Motion
Petals breathe in/out from the plasma phase drift (~0.02 rad/frame), the whole mandala rotates once every ~26 minutes, hue creeps continuously. Nothing jumps; all four phase speeds are irrational-ish ratios so the pattern never exactly repeats.

# 002 P4M Quilt

## Look
A "bathroom-tile kaleidoscope": the p4m wallpaper group folds a drifting four-wave plasma into a perfectly mirrored quilt of clover-and-star tiles. The cell size slowly breathes so the quilt zooms in and out while the lattice glides diagonally.

## Math
- p4m fold (wallpaper *442): u = tri(x mod 2L), v = tri(y mod 2L) (triangle-wave mirror both axes), then swap so u >= v (diagonal fold) — fundamental domain is the 45-45-90 triangle, 1/8 of the cell.
- Cell: L(t) = 52 + 18·sin(0.004t); lattice drift (x,y) += (0.16t, 0.09t).
- Source in the triangle: sum of 4 sines — sin(0.11u), sin(0.13v), sin(0.065(u+v)), sin(0.17·dist(u,v to cell center)) with independent slow phase rates.
- Color: cosine palette a=(.50,.46,.50) b=(.48,.44,.42) d=(0,.15,.42) — orange/blue/cream jewel tones.

## Integer ARM64 plan
- With L locked to powers of two the whole fold is `u = x & (2L−1); if u >= L: u = 2L−1−u` twice + one compare/swap — ~6 ops/pixel, no multiply (geometry.md §2). For breathing L, step L through integers and precompute a 512-entry mirror-repeat LUT per frame instead.
- Plasma = three axis-aligned waves: sin along u and along v are 1-D — precompute two 256-entry row/column tables per frame; only the (u+v) and distance terms need per-pixel adds + table lookups. Distance term uses squared distance into a 256-entry sqrt-fold LUT, no isqrt.
- All sines from the shared Q15 quarter-wave table via BAM16 phases; final v is 8-bit index into the DAC palette; palette phase roll = DAC rotation, free.

## Palette pairing
Warm amber/burnt-orange against deep ultramarine, cream highlights — a Persian-carpet pairing that keeps figure/ground bold at every cycle phase.

## Motion
Lattice drifts ~0.19 px/frame diagonally; cell breathes over a ~26 s period; four plasma phases crawl at 0.005–0.010/frame; palette rolls at 0.0005/frame. Everything is continuous — no pops.

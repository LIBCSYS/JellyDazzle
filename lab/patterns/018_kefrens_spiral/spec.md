# 018 Kefrens Spiral

## Look
Kefrens/Alcatraz bars with the scanline axis swapped for radius: a never-cleared angular line buffer is stamped and re-stamped as radius grows outward, so the bar's wander smears into taffy tendrils radiating from the center, mirrored 8 ways. Hue is written at stamp time and frozen into each streak, giving a spectrum-swept starburst.

## Math
- One line buffer `melt[360]` over the folded half-wedge (4-fold → π/4 span), plus a parallel hue buffer.
- For r = 0…204: bar angle `θ(r,t) = 2.2·sin(0.045r + 0.012t) + 1.4·sin(0.023r − 0.007t) + 0.004t`, folded into the wedge (bounces off seams); stamp a triangular bump (half-width 15 bins): `melt = max(melt, bump)` inside the bump, `melt ·= 0.9865` outside (the melt); bump core writes `hue = (0.004r + 0.0011t) mod 1`.
- Row r of the polar image is the buffer state after stamp r; screen pixel = `rows[r_pix][bin(fa)]`.
- `val = melt^0.85·(0.45 + 0.55(1 − r/230)) + 0.03`, `sat = 0.9 − 0.25·melt`.

## Integer ARM64 plan
- Direct port of the DOS software Kefrens loop, radius-major: 205 iterations × (2 `sin_tab` reads + a ~30-byte stamp + a decay pass over 360 bytes). Decay ×0.9865 ≈ `x − (x>>6) − (x>>8)` — shifts and subtracts, or one 256-byte decay LUT.
- The polar image is built row-by-row into a (205×360) byte buffer (72 KB — fits L2 easily; the whole inner loop is byte ops and NEON-vectorizable 16 bins at a time).
- Screen resolve through the precomputed polar `fold_map`: `pix = pal[hue_rows[idx]][melt_rows[idx]]` — two loads per pixel; the val/sat shaping is baked into a 256×16 palette table indexed by (hue byte, melt>>4).
- Angle fold of θ: wedge modulo via multiply-shift; the bounce (`w−fa`) is a compare + subtract.

## Palette pairing
Full-spectrum hue sweep laid down along radius (≈ one wheel per 250 px) over near-black; saturation dips slightly at streak cores so bright tendrils glint. Reads as a peacock starburst — many hues, but ordered by radius so it stays cohesive, never confetti.

## Motion
The bar's two spatial sines crawl (0.012 and 0.007 phase/frame) so tendrils slowly reshape like pulled taffy; the constant 0.004t term swings the whole burst around the center (~26 min/rev). Streak decay makes abandoned regions fade over ~2 s of radius — continuous melt, zero strobe.

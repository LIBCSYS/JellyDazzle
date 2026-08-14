# 010 Mosaic Quilt

## Look
A wall of hand-glazed tiles: every 60-px cell is its own tiny 4-fold mirror kaleidoscope, alternate cells add a diagonal fold (p4g-style checkering), and per-cell phase offsets make each tile a different diamond/clover motif. The quilt glides diagonally while a gold→teal→mint palette wave washes across it.

## Math
- Cell grid: (gx,gy) = floor((x,y)+drift)/L, in-cell coords centered, then u=|u|, v=|v| — per-cell D2×D2 mirror (4-fold). On odd (gx+gy) parity additionally (uu,vv) = (max,min) — the diagonal fold, giving the p4g-like alternation.
- Per-cell phase ph = 2.5·sin(1.7·gx + 2.3·gy) — deterministic variety, no RNG.
- Source: v = 0.5 + 0.30·sin(0.22·(uu+vv) − 0.010t + ph) + 0.18·sin(0.19·(uu−vv) + 0.006t) + 0.10·sin(0.24·hypot(uu,vv) + ph − 0.007t) — diamond rings, diagonal weave, round rings.
- Hue = 0.55·v + 0.045·(gx+gy) + 0.0007t through a c=(1,1,.5) cosine palette — the halved blue frequency yields the antique gold/teal drift.

## Integer ARM64 plan
- L = 64 (power of two): gx = x>>6, u = (x & 63) − 32, u=|u| — the whole double fold is shifts/abs/compare-swap, zero multiplies (geometry.md §2 p4m machinery per cell).
- Parity = (gx^gy) & 1 selects the swap branch — branchless with csel/NEON masks.
- ph per cell: 16-bit hash-free table — precompute one row of sin(1.7gx) and column of sin(2.3gy) per frame (≤ 6 entries each at 320×240), sum per cell; cells are 64-px so this is per-tile, not per-pixel.
- Three source sines: (uu+vv) and (uu−vv) are adds; hypot via alpha-max-beta-min; BAM phases + quarter-wave lookups. All per-pixel multiplies ≈ 4.
- Hue add (gx+gy)·k is per-tile constant; final byte indexes the DAC; the palette wave = slow DAC phase roll, free.

## Palette pairing
Antique gold and ink-black tiles dissolving into deep teal then porcelain mint — a wave of glaze crossing the quilt; per-cell phase keeps neighboring tiles related but never identical.

## Motion
Quilt drifts (0.10, 0.06) px/frame; ring phases crawl at 0.006–0.010/frame so each tile's motif slowly turns inside-out; the palette wave crosses the screen in ~45 s. Calm, orderly, endlessly varied.

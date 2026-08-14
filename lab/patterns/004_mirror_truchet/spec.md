# 004 Mirror Truchet

## Look
Neon Truchet ribbons — glowing quarter-circle arcs that link into meandering paths — seen through an 8-fold mirror fold, so the random tile field becomes a perfectly symmetric mandala of loops. Each tile carries its own hue; the ground is a deep navy radial gradient.

## Math
- Fold: rotate screen by 0.002t, then x=|x|, y=|y|, (fx,fy)=(max,min) — the exact D8 abs/swap fold, no trig per pixel beyond the global rotation.
- Truchet: scroll folded coords, tile size L=42; tile hash h = frac(sin(12.9898·tx + 78.233·ty)·43758.5453); if h ≥ 0.5 mirror u := L−u. Arc distance d = min(| |(u,v)| − L/2 |, | |(L−u,L−v)| − L/2 |); ribbon mask = smoothstep(1 − d/7).
- Color: ribbon hue = 0.11·tx + 0.07·ty + 0.0012t through a rainbow cosine palette; ground = dark cosine palette of screen radius; out = lerp(ground, ink, ribbon).

## Integer ARM64 plan
- D8 fold: `x=|x|; y=|y|; if y>x swap` — 3 ops (geometry.md §1 Regime A). Global rotation via per-frame Q15 (c,s) and two multiplies/pixel, or per-scanline incremental adds (dx,dy per step) for zero per-pixel multiplies.
- Tile: L a power of two (32 or 64) → tx = x>>k, in-tile u = x & (L−1). Hash: 16-bit `h = tx·0x9E37 + ty·0x79B9; h ^= h>>7` (geometry.md §9); bit 0 = orientation, bits 1–7 = hue offset.
- Arc test entirely in squared distances: precompute (R−w)² and (R+w)²; d1 = u·u + v·v compare — no sqrt. Ribbon softness from a small LUT indexed by clamped |d1 − R²| >> shift (avoids the true distance).
- Two palettes = two 128-entry halves of the 256-entry DAC; ribbon pixels index the top half by tile-hue, ground indexes the bottom half by radius — the lerp becomes a 1-bit select, dazzle-style.

## Palette pairing
Full-rainbow neon ink (each tile one hue, neighbors analogous) over near-black indigo ground — high contrast, no wash-out, reads like glass tubing.

## Motion
Global fold rotation ~1 rev / 52 s; folded plane scrolls (0.22, 0.13) px/frame so loops continuously reconnect at the mirror seams; hue field drifts at 0.0012/frame. No element changes faster than a slow crawl.

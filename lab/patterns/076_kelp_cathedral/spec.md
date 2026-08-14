# 076 Kelp Cathedral

## Look
A kelp forest grows toward the surface through teal water, god-rays slanting down between green and crimson stalks ringed with curved fronds and gold shimmer tips; faint background stalks add depth and bubbles rise slowly. It feels like standing on the sea floor looking up.

## Math
- Water: vertical gradient + rays `((0.5+0.5·sin(0.045x−0.006t+sin(0.01y)))⁴·(1−y/H)^1.6`.
- 17 stalks (35% dim "far" layer at 0.35 value): height `min(Hmax, 0.35·(t+160)·spd)`; sway `x = anchor + A·sin(0.028y+0.018t·spd+φ)·rise^1.2 + 4·sin(0.011y−0.011t+2φ)·rise`.
- Fronds every 24 px, alternating sides, 7-sample arcs `(±r, −0.55r·flex + 0.045r²)`, `flex = 0.6+0.4·sin(0.02t+…)`.
- Tips get pulsing gold splats `2.0+1.2·sin(0.025t)`; 20 bubbles rise on `(t·0.7+53k) mod (H+30)`.

## Integer ARM64 plan
- Rays: per-column sintab read raised to 4th power via two squarings (Q14), times a per-row depth ramp LUT — one mul per pixel or per 2×2 block.
- Stalks: per-row x from two sintab reads + mul by rise-ramp LUT; draw 3-px-wide vertical runs into accumulate buffer as heights extend (persistent growth, no redraw).
- Fronds = 7 fixed offset splats (sprite table); flex modulates the y-offset table index.
- Bubbles: 20 sprites on modular counters — adds only.

## Palette pairing
Water `(3,13,26)→(10,38,59)` with ray lift; kelp greens H 0.26–0.38 (V up with height), 20% crimson H 0.94 accents; frond gold-greens H 0.16–0.34; tips gold `(255,217,77)`.

## Motion
Stalks grow ~0.35 px/frame to full height in ~2 min then keep swaying (8–15 s periods, staggered); rays drift a full cycle in ~17 min; bubbles take ~6 s to cross. Everything oscillates, nothing snaps.

# 064 Starburst Forge

## Look
Twenty-four molten rays fan out from a dark core like a blacksmith's starburst, bending gently as they go, while glowing copper rings race down the rays into the center — sunburst and tunnel fused into one furnace mandala in ember red, orange, gold and white-hot.

## Math
- `rays = sin(12a + sweep + 0.55 sin(depth*0.045 + t*0.008))` — 24 lobes, sweep `= t*0.004`, the inner sine bows the rays into gentle S-curves near the core
- `rings = sin(depth*0.45 - t*0.5)` with `depth = 2200/(r+12)` — inward-flying copper rings (§5 of demoscene notes: copper bars promoted to radius)
- `idx = (0.62*rays + 0.38*rings)*118 + 128` → forge palette; shade `= r/(r+42)`

## Integer ARM64 plan
- Init tables: `ang12_tab[i]` (12*angle byte), `depth_tab[i]`, `shade_tab[i]`.
- Rays: `sin16[(ang12_tab[i]<<8) + sweep + bend]` where `bend = sin16[(depth_tab[i]*11) + p]>>10` — two chained table sines, no trig.
- Rings: `sin16[(depth_tab[i]*115 - fly)]` — one more lookup.
- Weighted sum with shifts (`(rays*5 + rings*3)>>3`), bias 128, clamp via 512-entry sat table → palette index.
- Every per-pixel op is add/shift/lookup; the only multiplies are by small constants (replaceable by shift-adds).

## Palette pairing
Fire loop biased dark at both ends so ray troughs read as black iron; white-hot occupies only ~10% of the palette, appearing exactly where a ring crest crosses a ray crest — sparks without strobing.

## Motion
Ray fan sweeps a full turn in ~4 minutes; rings fly inward at 0.5 idx/frame (ring period ≈ 12 s); ray-bend wobble period ≈ 26 s. Slow drift everywhere; highlights migrate rather than flash.

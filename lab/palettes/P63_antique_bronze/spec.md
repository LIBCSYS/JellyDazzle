# P63 Antique Bronze

**Class** `metallic` — A narrow warm or cold hue band riding a long, steep value ramp -- the specular curve of gold, copper, steel, bronze. Chroma stays modest; the shine comes from lightness, not colour. Must include a return leg (see RULE 3) or the cyclic wrap cliffs.

**Scheme** green-gold bronze 110-154 on a full value ramp; warm return

## Mood
Bronze that has been handled for a century — green-gold in the body, a hard polished hit on the high edge, warm grey in the shadow.

## Look
A narrow hue band riding a long steep value ramp, holding a PLATEAU near the top (the specular hit) before the neutral return leg closes the loop. The shine is lightness, not chroma: C_lit 0.21.

## Pattern pairing
Bevel, emboss and relief patterns; anything with implied thickness, where a flat chroma ramp reads as paper.

## Swatch

![swatch](swatch.png)

Top band: the 21 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 21 | · |
| `hue_bins` | 2 | · |
| `hue_arc` | 22.992 | 0 .. 60 |
| `accent_frac` | 0.0 | · |
| `C_mean` | 0.116 | · |
| `C_lit` | 0.211 | 0.2 .. 0.6 |
| `C_sd` | 0.081 | · |
| `C_max` | 0.268 | · |
| `L_mean` | 0.591 | · |
| `L_sd` | 0.281 | 0.22 .. 0.4 |
| `L_range` | 0.881 | 0.75 .. 1.0 |
| `dark_frac` | 0.238 | · |
| `light_frac` | 0.286 | 0.08 .. 0.4 |
| `neutral_frac` | 0.571 | · |
| `max_step` | 12.84 | · |
| `step_ratio` | 1.51 | 0 .. 2.8 |

Gate: **PASS** — fit=1.00 nn=P87_hazard_tape d=0.213

## Colors (21)

`040400` `140b08` `322824` `544845` `786c68` `9e918d` `c6b8b4` `efe1dc` `f5fcf5` `e8f2e8` `dbe7da` `cddccd` `ccddc9` `acc4a4` `8faa7f` `75905c` `5f763c` `4a5c1f` `384309` `252b04` `131501`

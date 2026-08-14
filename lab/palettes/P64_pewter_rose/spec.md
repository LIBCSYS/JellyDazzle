# P64 Pewter Rose

**Class** `metallic` — A narrow warm or cold hue band riding a long, steep value ramp -- the specular curve of gold, copper, steel, bronze. Chroma stays modest; the shine comes from lightness, not colour. Must include a return leg (see RULE 3) or the cyclic wrap cliffs.

**Scheme** warm pewter 348-22 on a full value ramp; neutral return leg

## Mood
Old pewter with the faintest blush in it — black in the shadow, white on the rim, and a warmth you only see next to a true grey.

## Look
The least chromatic palette in the block: C_lit 0.21, an 18-degree hue arc, and all the drama carried by an L_range of 0.99. A first draft of this slot was a brass at hue 76; it cleared every gate when it was searched and then collided with `P73_brass_vigil` (d=0.155) when that palette was revised mid-flight, so it was re-searched against the live library and replaced.

## Pattern pairing
Specular and sheen patterns; survives compositing under other layers because it carries almost no hue to fight with.

## Swatch

![swatch](swatch.png)

Top band: the 21 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 21 | · |
| `hue_bins` | 2 | · |
| `hue_arc` | 18.331 | 0 .. 60 |
| `accent_frac` | 0.0 | · |
| `C_mean` | 0.121 | · |
| `C_lit` | 0.207 | 0.2 .. 0.6 |
| `C_sd` | 0.086 | · |
| `C_max` | 0.267 | · |
| `L_mean` | 0.575 | · |
| `L_sd` | 0.295 | 0.22 .. 0.4 |
| `L_range` | 0.921 | 0.75 .. 1.0 |
| `dark_frac` | 0.238 | · |
| `light_frac` | 0.286 | 0.08 .. 0.4 |
| `neutral_frac` | 0.571 | · |
| `max_step` | 13.47 | · |
| `step_ratio` | 1.52 | 0 .. 2.8 |

Gate: **PASS** — fit=1.00 nn=P45_hammered_copper d=0.236

## Colors (21)

`030001` `0b0403` `2a201c` `4c413e` `726662` `9a8d89` `c3b6b2` `e8d1d2` `efe1dc` `fff8f8` `f9eaeb` `f1dede` `eccfd2` `d9adb3` `c38c97` `ab6d7d` `915164` `74384d` `562138` `380d23` `1b010e`

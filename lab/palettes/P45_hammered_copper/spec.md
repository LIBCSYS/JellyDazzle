# P45 Hammered Copper

**Class** `metallic` — A narrow warm or cold hue band riding a long, steep value ramp -- the specular curve of gold, copper, steel, bronze. Chroma stays modest; the shine comes from lightness, not colour. Must include a return leg (see RULE 3) or the cyclic wrap cliffs.

**Scheme** copper 26-72 on a full value ramp; cool neutral return leg

## Mood
A copper vessel under a single lamp: black in the shadow, white on the rim, and every value between is the metal.

## Look
A narrow hue band riding a long steep value ramp, with a cool near-neutral return leg so the cyclic wrap has no cliff — the construction RULE 1 was written for.

## Pattern pairing
Specular, sheen and bevel patterns. The shine here is lightness rather than chroma, so it survives being composited underneath other layers.

## Swatch

![swatch](swatch.png)

Top band: the 20 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 20 | · |
| `hue_bins` | 1 | · |
| `hue_arc` | 11.096 | 0 .. 60 |
| `accent_frac` | 0.0 | · |
| `C_mean` | 0.175 | · |
| `C_lit` | 0.321 | 0.2 .. 0.6 |
| `C_sd` | 0.135 | · |
| `C_max` | 0.42 | · |
| `L_mean` | 0.511 | · |
| `L_sd` | 0.283 | 0.22 .. 0.4 |
| `L_range` | 0.927 | 0.75 .. 1.0 |
| `dark_frac` | 0.3 | · |
| `light_frac` | 0.2 | 0.08 .. 0.4 |
| `neutral_frac` | 0.45 | · |
| `max_step` | 13.53 | · |
| `step_ratio` | 1.42 | 0 .. 2.8 |

Gate: **PASS** — fit=1.00 nn=P24_obsidian_magma d=0.202

## Colors (20)

`020000` `010508` `182226` `394349` `5d686e` `838f95` `acb9bf` `d7e4ea` `fef7f1` `fdd6ba` `f3b68a` `e39963` `d17c3e` `bb611c` `9b4e15` `7d3b0e` `5f2a08` `431a04` `280c01` `0f0200`

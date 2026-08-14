# P86 Verdigris Steel

**Class** `metallic` — A narrow warm or cold hue band riding a long, steep value ramp -- the specular curve of gold, copper, steel, bronze. Chroma stays modest; the shine comes from lightness, not colour. Must include a return leg (see RULE 3) or the cyclic wrap cliffs.

**Scheme** verdigris steel 182-228 on a full value ramp; warm return leg

## Mood
Oxidised bronze on a cold day — tin, patina, and the blue-green that lives in weathered metal.

## Look
The cold twin of the warm metallics: the same steep specular value ramp on the opposite side of the wheel, with a warm near-neutral return leg instead of a cool one.

## Pattern pairing
Bevel, edge and specular patterns; also anything that should read as machined rather than painted.

## Swatch

![swatch](swatch.png)

Top band: the 20 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 20 | · |
| `hue_bins` | 2 | · |
| `hue_arc` | 14.415 | 0 .. 60 |
| `accent_frac` | 0.0 | · |
| `C_mean` | 0.138 | · |
| `C_lit` | 0.252 | 0.2 .. 0.6 |
| `C_sd` | 0.101 | · |
| `C_max` | 0.326 | · |
| `L_mean` | 0.52 | · |
| `L_sd` | 0.278 | 0.22 .. 0.4 |
| `L_range` | 0.918 | 0.75 .. 1.0 |
| `dark_frac` | 0.3 | · |
| `light_frac` | 0.2 | 0.08 .. 0.4 |
| `neutral_frac` | 0.5 | · |
| `max_step` | 13.12 | · |
| `step_ratio` | 1.4 | 0 .. 2.8 |

Gate: **PASS** — fit=1.00 nn=P36_fen_water d=0.201

## Colors (20)

`000101` `0d0504` `2b211e` `4d423e` `726662` `998c88` `c1b4b0` `ecded9` `edfbfe` `aeebf7` `7ed6e4` `48c1d0` `2aa9b5` `228f98` `1a777c` `135f62` `0c4849` `063233` `021e1d` `000b0b`

# P88 Ultraviolet Press

**Class** `stark` — Two or three values, almost no midtones, chroma near zero or slammed. Graphic and hard -- the opposite pole from pastel. The only class permitted a high step_ratio: the cliff IS the look.

**Scheme** near-black / chalk white / ultraviolet

## Mood
A hand-pressed poster in two inks, and someone ran a violet plate over the top of it slightly out of register.

## Look
The same two-plateau structure as the other starks with a violet accent and faintly violet-tinted blacks. No midtone anywhere.

## Pattern pairing
Grid, wire and edge-detect patterns; reads as print rather than as light.

## Swatch

![swatch](swatch.png)

Top band: the 17 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 17 | · |
| `hue_bins` | 1 | 1 .. 3 |
| `hue_arc` | 3.949 | · |
| `accent_frac` | 0.0 | · |
| `C_mean` | 0.143 | 0.0 .. 0.3 |
| `C_lit` | 0.732 | · |
| `C_sd` | 0.276 | · |
| `C_max` | 0.856 | · |
| `L_mean` | 0.526 | · |
| `L_sd` | 0.387 | 0.28 .. 0.5 |
| `L_range` | 0.989 | 0.85 .. 1.0 |
| `dark_frac` | 0.412 | · |
| `light_frac` | 0.412 | · |
| `neutral_frac` | 0.824 | 0.35 .. 1.0 |
| `max_step` | 66.01 | · |
| `step_ratio` | 5.26 | 0 .. 5.0 |

Gate: **PASS** — fit=0.97 nn=P46_newsprint_siren d=0.340

## Colors (17)

`000000` `000001` `6600c4` `8f23ff` `9c72ff` `fbfbfe` `f6f7f9` `f2f2f5` `ededf0` `e9e9eb` `e4e4e7` `d0d1d3` `151619` `0d0d10` `07070a` `030405` `010102`

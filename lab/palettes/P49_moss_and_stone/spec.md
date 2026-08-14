# P49 Moss & Stone

**Class** `earth` — Ochre, clay, moss, bark, stone. Warm-biased mid hues at low chroma and mid lightness -- the only class that is deliberately muted rather than dark or pale.

**Scheme** olive -> sage -> slate, all low chroma

## Mood
A cold wall in a wet wood — lichen, slate, olive and the grey-green of old rain.

## Look
The cool half of the earth family, chroma kept under a third and lightness parked in the middle. Runs lighter than Kiln & Clay so the two earths separate on tone as well as hue.

## Pattern pairing
Terrain, sediment and crack patterns; pairs well with the warm earth set when the scheduler cross-fades between them.

## Swatch

![swatch](swatch.png)

Top band: the 19 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 19 | · |
| `hue_bins` | 3 | · |
| `hue_arc` | 63.634 | 40 .. 160 |
| `accent_frac` | 0.137 | · |
| `C_mean` | 0.202 | · |
| `C_lit` | 0.206 | 0.12 .. 0.4 |
| `C_sd` | 0.025 | 0.0 .. 0.16 |
| `C_max` | 0.24 | · |
| `L_mean` | 0.527 | 0.38 .. 0.65 |
| `L_sd` | 0.137 | 0.12 .. 0.28 |
| `L_range` | 0.481 | · |
| `dark_frac` | 0.053 | · |
| `light_frac` | 0.0 | · |
| `neutral_frac` | 0.0 | · |
| `max_step` | 8.66 | · |
| `step_ratio` | 1.57 | 0 .. 2.4 |

Gate: **PASS** — fit=1.00 nn=P37_ripening_field d=0.260

## Colors (19)

`2f2905` `403d0d` `515222` `616837` `667952` `727f4d` `848b5e` `a39e6b` `9cbd8a` `93af7b` `7fa67b` `829763` `648f6b` `4b785c` `4a6648` `33624d` `2f543e` `1d4c3e` `164134`

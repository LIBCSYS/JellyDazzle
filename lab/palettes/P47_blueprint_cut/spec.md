# P47 Blueprint Cut

**Class** `stark` — Two or three values, almost no midtones, chroma near zero or slammed. Graphic and hard -- the opposite pole from pastel. The only class permitted a high step_ratio: the cliff IS the look.

**Scheme** near-black / chalk white / cold cyan

## Mood
Drafting-table contrast: near-black paper, chalk-white line, and one cold cyan cut through both.

## Look
The cool twin of Newsprint Siren — the same two-plateau structure with the opposite accent, so the two starks cannot be confused on screen.

## Pattern pairing
Grid, wire and edge-detect patterns. Reads as technical drawing rather than poster art.

## Swatch

![swatch](swatch.png)

Top band: the 17 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 17 | · |
| `hue_bins` | 1 | 1 .. 3 |
| `hue_arc` | 3.566 | · |
| `accent_frac` | 0.0 | · |
| `C_mean` | 0.077 | 0.0 .. 0.3 |
| `C_lit` | 0.301 | · |
| `C_sd` | 0.107 | · |
| `C_max` | 0.356 | · |
| `L_mean` | 0.527 | · |
| `L_sd` | 0.387 | 0.28 .. 0.5 |
| `L_range` | 0.99 | 0.85 .. 1.0 |
| `dark_frac` | 0.412 | · |
| `light_frac` | 0.412 | · |
| `neutral_frac` | 0.824 | 0.35 .. 1.0 |
| `max_step` | 65.73 | · |
| `step_ratio` | 5.54 | 0 .. 5.0 |

Gate: **PASS** — fit=0.94 nn=house:ice d=0.302

## Colors (17)

`000000` `000001` `020407` `05080d` `090e13` `12171c` `cdd1d6` `e1e5e9` `e6eaed` `ebeef2` `eff3f7` `f4f8fb` `f9fcff` `00a3c0` `00829e` `005d6f` `000203`

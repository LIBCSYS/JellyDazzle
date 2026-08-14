# P46 Newsprint Siren

**Class** `stark` — Two or three values, almost no midtones, chroma near zero or slammed. Graphic and hard -- the opposite pole from pastel. The only class permitted a high step_ratio: the cliff IS the look.

**Scheme** black / white / siren red

## Mood
Black ink, white paper, and one siren-red slash across it. No midtones and no apology.

## Look
Two value plateaus with almost nothing between them plus three red anchors. The cliff IS the look — stark is the one class the smoothness budget is relaxed for.

## Pattern pairing
Hard-edged geometry: grids, bars, halftone, scanline work. Soft-edged patterns should avoid this one.

## Swatch

![swatch](swatch.png)

Top band: the 16 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 16 | · |
| `hue_bins` | 1 | 1 .. 3 |
| `hue_arc` | 3.938 | · |
| `accent_frac` | 0.0 | · |
| `C_mean` | 0.12 | 0.0 .. 0.3 |
| `C_lit` | 0.639 | · |
| `C_sd` | 0.251 | · |
| `C_max` | 0.689 | · |
| `L_mean` | 0.557 | · |
| `L_sd` | 0.378 | 0.28 .. 0.5 |
| `L_range` | 0.991 | 0.85 .. 1.0 |
| `dark_frac` | 0.375 | · |
| `light_frac` | 0.438 | · |
| `neutral_frac` | 0.812 | 0.35 .. 1.0 |
| `max_step` | 66.05 | · |
| `step_ratio` | 5.04 | 0 .. 5.0 |

Gate: **PASS** — fit=1.00 nn=P45_hammered_copper d=0.306

## Colors (16)

`000000` `010101` `040404` `080808` `0d0d0d` `161616` `d1d1d1` `e4e4e4` `e9e9e9` `eeeeee` `f2f2f2` `f7f7f7` `fcfcfc` `ff3e4e` `da001e` `9e001a`

# P71 Jade Lantern

**Class** `mono_accent` — One hue family carries the whole frame; a single opposing accent appears in under a fifth of it. Restraint is the point.

**Scheme** jade 138-150 field; ember orange 28-40 accent

## Mood
A jade room lit from one corner — everything in it is green stone except the single paper lantern burning at the far end.

## Look
Eleven stops of one jade family from near-black to frost, a near-neutral descent closing the loop, and exactly two ember anchors holding under a fifth of the chroma. Jade sits high in the sRGB gamut, so the accent had to be scaled DOWN to keep the class ceiling — see `b_mono2`.

## Pattern pairing
Flow, caustic and interference fields — anything with a single dominant current. The ember reads as a spark inside the field, never as a competing subject.

## Swatch

![swatch](swatch.png)

Top band: the 19 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 19 | · |
| `hue_bins` | 2 | 1 .. 3 |
| `hue_arc` | 106.125 | · |
| `accent_frac` | 0.151 | 0.04 .. 0.22 |
| `C_mean` | 0.252 | · |
| `C_lit` | 0.43 | 0.3 .. 0.75 |
| `C_sd` | 0.188 | · |
| `C_max` | 0.572 | · |
| `L_mean` | 0.47 | · |
| `L_sd` | 0.255 | · |
| `L_range` | 0.861 | 0.6 .. 1.0 |
| `dark_frac` | 0.316 | · |
| `light_frac` | 0.053 | · |
| `neutral_frac` | 0.368 | · |
| `max_step` | 15.08 | · |
| `step_ratio` | 1.37 | 0 .. 2.6 |

Gate: **PASS** — fit=1.00 nn=P36_fen_water d=0.220

## Colors (19)

`000000` `010301` `182018` `3b433b` `626b61` `b65e3f` `8b958b` `e79c7e` `b7c1b6` `99e7a6` `69d17c` `2fbb50` `1b9f39` `148327` `0d6818` `074e09` `053503` `031e01` `010900`

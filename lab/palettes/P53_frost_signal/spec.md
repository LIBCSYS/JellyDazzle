# P53 Frost Signal

**Class** `mono_accent` — One hue family carries the whole frame; a single opposing accent appears in under a fifth of it. Restraint is the point.

**Scheme** ice blue 204-216 field; one red signal at 0

## Mood
A cold room in full daylight: pale blue walls, frost on the glass, and a single red indicator burning on a panel.

## Look
The high-key mono of the three — ground lifted to L 0.14 and ceiling pushed to 0.97, so dark_frac falls to 0.19 where the other two sit near 0.33. Same restraint, different weather.

## Pattern pairing
Caustics, frost and ripple patterns; anything that wants a light field with one point of attention in it.

## Swatch

![swatch](swatch.png)

Top band: the 21 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 21 | · |
| `hue_bins` | 3 | 1 .. 3 |
| `hue_arc` | 156.155 | · |
| `accent_frac` | 0.189 | 0.04 .. 0.22 |
| `C_mean` | 0.187 | · |
| `C_lit` | 0.305 | 0.3 .. 0.75 |
| `C_sd` | 0.124 | · |
| `C_max` | 0.403 | · |
| `L_mean` | 0.562 | · |
| `L_sd` | 0.244 | · |
| `L_range` | 0.829 | 0.6 .. 1.0 |
| `dark_frac` | 0.191 | · |
| `light_frac` | 0.191 | · |
| `neutral_frac` | 0.381 | · |
| `max_step` | 14.53 | · |
| `step_ratio` | 1.58 | 0 .. 2.6 |

Gate: **PASS** — fit=1.00 nn=P17_peacock_court d=0.201

## Colors (21)

`000c0e` `0a1618` `2c393c` `526063` `7b8a8d` `b7617d` `d08094` `a6b6b9` `d4e5e8` `e6f9ff` `a4ecfd` `3edefc` `27c6e2` `21afc6` `1b98aa` `168190` `106b77` `0b565f` `064249` `032f33` `011c20`

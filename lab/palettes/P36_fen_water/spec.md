# P36 Fen Water

**Class** `analogous` — Neighbouring hues only -- a continuous slice of the wheel, no more than a third of a turn. Warm and cool instances are separate palettes and the diversity gate keeps them apart.

**Scheme** green 108-132 -> teal -> cyan, roughly a quarter turn

## Mood
Standing water over moss — everything green, teal and cyan, lit from under the surface.

## Look
A quarter-turn of the wheel with no warm escape, closing through a deep blue-green shadow instead of snapping back to black.

## Pattern pairing
Caustics, ripple and cellular-noise patterns. The tight hue slice leaves brightness to do all the storytelling.

## Swatch

![swatch](swatch.png)

Top band: the 26 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 26 | · |
| `hue_bins` | 3 | 3 .. 5 |
| `hue_arc` | 68.07 | 50 .. 125 |
| `accent_frac` | 0.199 | 0.0 .. 0.32 |
| `C_mean` | 0.242 | · |
| `C_lit` | 0.3 | 0.3 .. 0.85 |
| `C_sd` | 0.122 | · |
| `C_max` | 0.395 | · |
| `L_mean` | 0.48 | · |
| `L_sd` | 0.246 | · |
| `L_range` | 0.816 | 0.6 .. 1.0 |
| `dark_frac` | 0.269 | · |
| `light_frac` | 0.115 | · |
| `neutral_frac` | 0.115 | · |
| `max_step` | 10.56 | · |
| `step_ratio` | 1.62 | 0 .. 2.4 |

Gate: **PASS** — fit=1.00 nn=P20_koi_pond_dusk d=0.267

## Colors (26)

`010100` `030400` `0d1100` `151e01` `1c2d02` `1e3d03` `184f05` `09611a` `0d7135` `11824c` `148b5d` `179a73` `1aa889` `1eb79f` `21c6b6` `27d5ce` `62dfe1` `8aeaf2` `a1d3cb` `86b1a4` `6b907f` `53705d` `3c523d` `263520` `121a06` `050500`

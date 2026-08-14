# P74 Violet & Wheat

**Class** `duotone` — Exactly two hue poles, 110-180 deg apart, each with a full ramp. No third family. Reads as a two-ink print.

**Scheme** violet 280-296 vs wheat gold 70-86

## Mood
Two inks that should not work together and do: a cold violet and a dry wheat gold, with nothing between them.

## Look
Two full ramps a little over 150 degrees apart, meeting only at the shared floor and ceiling so the loop closes at both ends and no third hue family ever appears.

## Pattern pairing
Two-body patterns — duelling attractors, reaction-diffusion, split fields. The pole pair keeps "us and them" legible.

## Swatch

![swatch](swatch.png)

Top band: the 18 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 18 | · |
| `hue_bins` | 2 | 2 .. 4 |
| `hue_arc` | 152.793 | 110 .. 250 |
| `accent_frac` | 0.363 | · |
| `C_mean` | 0.353 | · |
| `C_lit` | 0.433 | 0.35 .. 0.9 |
| `C_sd` | 0.197 | · |
| `C_max` | 0.738 | · |
| `L_mean` | 0.456 | · |
| `L_sd` | 0.261 | · |
| `L_range` | 0.809 | 0.6 .. 1.0 |
| `dark_frac` | 0.333 | · |
| `light_frac` | 0.111 | · |
| `neutral_frac` | 0.056 | · |
| `max_step` | 15.64 | · |
| `step_ratio` | 1.45 | 0 .. 2.6 |

Gate: **PASS** — fit=1.00 nn=P40_harvest_storm d=0.206

## Colors (18)

`000005` `030000` `130900` `311d02` `533405` `774c0b` `9e6512` `c77f19` `f19b2c` `fdc58c` `cbccfd` `a6a6fb` `837efa` `6452f4` `4a26d7` `320da1` `1b0566` `070130`

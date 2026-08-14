# P80 Sodium Rose

**Class** `neon_on_black` — Half the ramp is near-black; what is lit is at maximum chroma. Electric, and the dark floor is what makes it read that way.

**Scheme** void floor; rose 334-342 + sodium gold 74-84 at the gamut edge

## Mood
A motorway at three in the morning: sodium lamps overhead, a rose neon sign at the exit, and nothing lit in between.

## Look
Rose sits at the sRGB chroma maximum, so this one carries the highest C_max of the three neons while the gold side stays comparatively restrained — a deliberately unbalanced pair.

## Pattern pairing
Filament, spark and glow-decay patterns. Very strong under additive layering, where the black floor absorbs the accumulation instead of blowing out.

## Swatch

![swatch](swatch.png)

Top band: the 25 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 25 | · |
| `hue_bins` | 2 | 2 .. 5 |
| `hue_arc` | 100.326 | · |
| `accent_frac` | 0.347 | · |
| `C_mean` | 0.343 | · |
| `C_lit` | 0.539 | 0.6 .. 1.0 |
| `C_sd` | 0.285 | · |
| `C_max` | 0.92 | 0.8 .. 1.0 |
| `L_mean` | 0.438 | 0.18 .. 0.48 |
| `L_sd` | 0.278 | · |
| `L_range` | 0.88 | · |
| `dark_frac` | 0.4 | 0.4 .. 0.7 |
| `light_frac` | 0.08 | · |
| `neutral_frac` | 0.36 | · |
| `max_step` | 16.4 | · |
| `step_ratio` | 1.91 | 0 .. 3.0 |

Gate: **PASS** — fit=0.93 nn=P72_damson_hour d=0.196

## Colors (25)

`000000` `020102` `090408` `140d13` `21171f` `590050` `8b007d` `b500a3` `de00c8` `fb00e2` `ff32e6` `ff83ea` `ffb5f0` `ffce8d` `fdaa00` `f7a600` `cf8a00` `a76f00` `7f5300` `563700` `251c11` `271700` `181108` `0d0702` `040200`

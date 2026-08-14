# P50 Carnival at Midnight

**Class** `full_spectrum` — The whole wheel. Legitimate as a CLASS -- illegitimate as half the library, which is what v2.0 was. Capped at 4 of 60.

**Scheme** all twelve hue bins over a dark ground

## Mood
The whole wheel, but seen at night — every hue present and all of them sitting on black instead of on white.

## Look
Twelve gamut-edge hues over a void floor. The one full_spectrum slot in this batch, and deliberately the dark one: the class is rotation-invariant, so tone is the only thing that can tell it apart from the seventeen rainbows already in the library.

This palette is exempt from the within-class hue floor, and that exemption is a conflict between the spec and its own code. `palettes.md` 7 states that full_spectrum "is rotation-invariant by definition" and that its members "must be separated ENTIRELY by tone" — but `palette_score.accept()` applies `MIN_HUE_EMD_SAME_CLASS = 1.2` to every class alike. Measured: a 4,900-point sweep of rotation, chroma skew, ground depth and value amplitude reaches a maximum hue EMD of **0.93 bins** against the four full-spectrum palettes already shipped. The floor is unreachable for this class, exactly as the prose predicted, so the prose governs: this palette is gated on distance alone, and clears it at d=0.21 against a 0.16 floor.

## Pattern pairing
Confetti, particle and shard patterns, where per-element hue variety is the subject. Use sparingly — this class is capped at 4 of 60 for a reason.

## Swatch

![swatch](swatch.png)

Top band: the 19 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 19 | · |
| `hue_bins` | 10 | 9 .. 12 |
| `hue_arc` | 270.057 | 240 .. 360 |
| `accent_frac` | 0.684 | · |
| `C_mean` | 0.325 | · |
| `C_lit` | 0.471 | 0.45 .. 0.95 |
| `C_sd` | 0.237 | · |
| `C_max` | 0.854 | · |
| `L_mean` | 0.39 | · |
| `L_sd` | 0.252 | · |
| `L_range` | 0.76 | 0.7 .. 1.0 |
| `dark_frac` | 0.368 | · |
| `light_frac` | 0.0 | · |
| `neutral_frac` | 0.105 | · |
| `max_step` | 36.96 | · |
| `step_ratio` | 2.57 | 0 .. 2.6 |

Gate: **PASS** — fit=1.00 nn=P14_ink-crimson d=0.210

## Colors (19)

`000000` `000002` `020105` `05020a` `0b0510` `130816` `1c0d1c` `4f0b8c` `3129f7` `1ea1e8` `27c6dc` `1da28f` `da7e1b` `fc8d7b` `f51f85` `890f84` `725b0d` `0d642e` `384305`

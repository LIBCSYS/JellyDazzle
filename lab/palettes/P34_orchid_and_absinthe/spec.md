# P34 Orchid & Absinthe

**Class** `duotone` — Exactly two hue poles, 110-180 deg apart, each with a full ramp. No third family. Reads as a two-ink print.

**Scheme** orchid violet 316-324 vs absinthe green 126-134

## Mood
A violet so deep it is nearly ink, arguing with a green so acid it hums.

## Look
Magenta-violet against yellow-green. The two ramps cross in lightness but never in hue, which is what keeps a duotone from collapsing into a gradient.

## Pattern pairing
Lattice, moire and lissajous work — the pole pair keeps two interfering families legible where a rainbow turns to mud.

## Swatch

![swatch](swatch.png)

Top band: the 18 anchors. Bottom band: the cyclic ramp `gen_tables.py` expands from them.

## Class metrics

| metric | value | class target |
|---|---|---|
| `n` | 18 | · |
| `hue_bins` | 2 | 2 .. 4 |
| `hue_arc` | 174.39 | 110 .. 250 |
| `accent_frac` | 0.458 | · |
| `C_mean` | 0.361 | · |
| `C_lit` | 0.43 | 0.35 .. 0.9 |
| `C_sd` | 0.141 | · |
| `C_max` | 0.605 | · |
| `L_mean` | 0.48 | · |
| `L_sd` | 0.24 | · |
| `L_range` | 0.797 | 0.6 .. 1.0 |
| `dark_frac` | 0.278 | · |
| `light_frac` | 0.111 | · |
| `neutral_frac` | 0.0 | · |
| `max_step` | 16.98 | · |
| `step_ratio` | 1.66 | 0 .. 2.6 |

Gate: **PASS** — fit=1.00 nn=P25_orchid_greenhouse d=0.207

## Colors (18)

`030005` `1d0123` `400348` `660971` `8d1c98` `ac45b3` `c76dca` `df97de` `f3c1f0` `b6d399` `90bd65` `6ba62b` `528b14` `3e700e` `2b5608` `1b3c04` `0c2501` `020f00`
